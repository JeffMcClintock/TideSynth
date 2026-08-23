// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.
//
// A minimal CLAP host, enough to LOAD a plugin and see what it does.
//
// WHY THIS EXISTS. BACKLOG S37 says a Linux CLAP resolves its module data from
// the folder it is installed in -- `~/.clap/Resources` -- which every other
// CLAP installed the same way shares. That was derived from BundleInfo's path
// logic and had never been observed, because this box has no CLAP host:
// clap-validator and clap-info are not installed, and Ardour 8.4 has no CLAP
// support at all (`strings` finds no clap_entry). So the row's own first step
// was "get a CLAP host", and the cheapest honest one is this: the CLAP entry
// ABI is a C struct in a header the build already fetches, so a host that
// loads, instantiates and activates a plugin is under 200 lines and needs no
// third-party binary.
//
// It deliberately does the MINIMUM that forces a plugin to resolve its
// resources: dlopen -> clap_entry->init -> factory -> create -> init ->
// activate. Resource loading happens inside the plugin's own init, so a probe
// that stops at enumeration would report success while proving nothing.
//
// Build (the CLAP headers come from the plugin build tree's FetchContent):
//   gcc -O0 -g -o clap_probe tools/clap_probe.c -ldl \
//       -Ibuild/_deps/clap-src/include
//
// Usage:
//   clap_probe <path-to.clap> [--activate] [--gui] [--state]
//
// --gui is the option that matters for S37. The resource lookup does NOT
// happen in the processor's init or activate -- measured: with the Resources
// folder deleted, both still return true and strace shows no lookup at all.
// It happens when the CONTROLLER is constructed, which in CLAP is the GUI
// extension's create(). A probe that stops at activate reports a healthy
// plugin while proving nothing about its resources.
//
// Exit codes: 0 loaded and (if asked) activated; 1 the plugin refused; 2 the
// file could not be loaded as a CLAP at all.

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <clap/clap.h>

static const void *host_get_extension(const struct clap_host *h, const char *id)
{
	(void)h; (void)id;
	// Deliberately nothing. A plugin that REQUIRES an extension to initialise
	// should say so rather than be handed a stub that hides the requirement.
	return NULL;
}
static void host_request_restart(const struct clap_host *h) { (void)h; }
static void host_request_process(const struct clap_host *h) { (void)h; }
static void host_request_callback(const struct clap_host *h) { (void)h; }

static const clap_host_t g_host = {
	.clap_version = CLAP_VERSION_INIT,
	.host_data = NULL,
	.name = "clap_probe",
	.vendor = "TIDE Synth",
	.url = "https://tidesynth.com",
	.version = "1.0",
	.get_extension = host_get_extension,
	.request_restart = host_request_restart,
	.request_process = host_request_process,
	.request_callback = host_request_callback,
};

// --- an in-memory ostream/istream, so state can be round-tripped ------------
// The state path is the last way a host can reach the controller without a
// window, and TIDE's document travels as a blob (BACKLOG S12) -- so if the
// controller is ever constructed headlessly, this is where it happens.
static uint8_t g_buf[1 << 22];
static size_t g_len, g_pos;

static int64_t ostream_write(const clap_ostream_t *st, const void *b, uint64_t n)
{
	(void)st;
	if (g_len + n > sizeof(g_buf)) n = sizeof(g_buf) - g_len;
	memcpy(g_buf + g_len, b, (size_t)n); g_len += (size_t)n; return (int64_t)n;
}
static int64_t istream_read(const clap_istream_t *st, void *b, uint64_t n)
{
	(void)st;
	size_t left = g_len - g_pos; if (n > left) n = left;
	memcpy(b, g_buf + g_pos, (size_t)n); g_pos += (size_t)n; return (int64_t)n;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: clap_probe <path-to.clap> [--activate]\n");
		return 2;
	}
	const char *path = argv[1];
	int do_activate = 0, do_gui = 0, do_state = 0;
	for (int i = 2; i < argc; ++i) {
		if (!strcmp(argv[i], "--activate")) do_activate = 1;
		else if (!strcmp(argv[i], "--gui")) do_gui = 1;
		else if (!strcmp(argv[i], "--state")) do_state = 1;
		else { fprintf(stderr, "unknown option %s\n", argv[i]); return 2; }
	}

	char real[PATH_MAX];
	if (!realpath(path, real)) { perror("realpath"); return 2; }
	printf("probe: %s\n", real);

	// RTLD_NOW so a missing symbol is an error here rather than a crash later.
	void *so = dlopen(real, RTLD_NOW | RTLD_LOCAL);
	if (!so) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }

	const clap_plugin_entry_t *entry =
		(const clap_plugin_entry_t *)dlsym(so, "clap_entry");
	if (!entry) { fprintf(stderr, "no clap_entry symbol: %s\n", dlerror()); return 2; }

	printf("clap_version: %u.%u.%u\n", entry->clap_version.major,
	       entry->clap_version.minor, entry->clap_version.revision);

	// The path handed to init() is what a real host passes; GMPI resolves its
	// own module path via dladdr, so this is not the thing under test -- but
	// passing the wrong one would be a confound of our own making.
	if (!entry->init(real)) { fprintf(stderr, "entry->init returned false\n"); return 1; }

	const clap_plugin_factory_t *factory =
		(const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
	if (!factory) { fprintf(stderr, "no plugin factory\n"); entry->deinit(); return 1; }

	uint32_t count = factory->get_plugin_count(factory);
	printf("plugins: %u\n", count);
	if (count == 0) { entry->deinit(); return 1; }

	int rc = 0;
	for (uint32_t i = 0; i < count; ++i) {
		const clap_plugin_descriptor_t *d = factory->get_plugin_descriptor(factory, i);
		if (!d) { printf("  [%u] <null descriptor>\n", i); rc = 1; continue; }
		printf("  [%u] id=%s name=%s vendor=%s version=%s\n",
		       i, d->id ? d->id : "?", d->name ? d->name : "?",
		       d->vendor ? d->vendor : "?", d->version ? d->version : "?");

		const clap_plugin_t *p = factory->create_plugin(factory, &g_host, d->id);
		if (!p) { printf("      create_plugin FAILED\n"); rc = 1; continue; }

		// THIS is the call that makes the probe worth running: the plugin
		// resolves its module data here, so a missing Resources folder shows
		// up as the plugin's own diagnostics on stdout.
		if (!p->init(p)) { printf("      plugin->init FAILED\n"); p->destroy(p); rc = 1; continue; }
		printf("      init OK\n");

		if (do_activate) {
			if (!p->activate(p, 48000.0, 1, 512))
				{ printf("      activate FAILED\n"); rc = 1; }
			else { printf("      activate OK (48k, 1..512)\n"); p->deactivate(p); }
		}

		if (do_gui) {
			const clap_plugin_gui_t *gui =
				(const clap_plugin_gui_t *)p->get_extension(p, CLAP_EXT_GUI);
			if (!gui) {
				printf("      no %s extension\n", CLAP_EXT_GUI);
				rc = 1;
			} else {
				// Ask about every windowing API this platform could use, and
				// about the plugin's own preference. A host is entitled to
				// believe these answers and skip the GUI entirely.
				static const char *apis[] = { CLAP_WINDOW_API_X11, CLAP_WINDOW_API_WAYLAND,
				                              CLAP_WINDOW_API_WIN32, CLAP_WINDOW_API_COCOA };
				for (unsigned k = 0; k < sizeof(apis)/sizeof(apis[0]); ++k)
					printf("      is_api_supported(%-8s floating=false)=%d  (floating=true)=%d\n",
					       apis[k],
					       gui->is_api_supported ? gui->is_api_supported(p, apis[k], false) : -1,
					       gui->is_api_supported ? gui->is_api_supported(p, apis[k], true)  : -1);
				const char *pref = NULL; bool pref_float = false;
				if (gui->get_preferred_api && gui->get_preferred_api(p, &pref, &pref_float))
					printf("      get_preferred_api -> %s floating=%d\n", pref ? pref : "?", pref_float);
				else
					printf("      get_preferred_api -> (declined)\n");

				const char *api = CLAP_WINDOW_API_X11;
				// create() is what constructs the controller, and the
				// controller is what reads the bundle's Resources folder.
				if (!gui->create(p, api, false)) {
					printf("      gui->create FAILED\n");
					rc = 1;
				} else {
					uint32_t w = 0, h = 0;
					if (gui->get_size && gui->get_size(p, &w, &h))
						printf("      gui created, size %ux%u\n", w, h);
					else
						printf("      gui created, size unavailable\n");
					gui->destroy(p);
				}
			}
		}
		if (do_state) {
			const clap_plugin_state_t *stx =
				(const clap_plugin_state_t *)p->get_extension(p, CLAP_EXT_STATE);
			if (!stx) { printf("      no %s extension\n", CLAP_EXT_STATE); }
			else {
				g_len = g_pos = 0;
				clap_ostream_t os = { .ctx = NULL, .write = ostream_write };
				if (!stx->save(p, &os)) printf("      state->save FAILED\n");
				else {
					printf("      state->save OK, %zu bytes\n", g_len);
					clap_istream_t is = { .ctx = NULL, .read = istream_read };
					printf("      state->load %s\n", stx->load(p, &is) ? "OK" : "FAILED");
				}
			}
		}
		p->destroy(p);
	}

	entry->deinit();
	return rc;
}
