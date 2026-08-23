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
//   clap_probe <path-to.clap> [--activate] [--gui] [--state] [--embed SECS]
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

// --embed turns this from a loader into a real X11 host: it creates a parent
// window, offers the two extensions an X11 editor cannot live without
// (posix-fd and timer), embeds the plugin, and then PUMPS -- polling the fd the
// plugin registered and ticking its timer. BACKLOG S43(ii). Without the pump an
// embedded editor is a window that never paints, so a probe that stops at
// set_parent would report success and show nothing.
#include <X11/Xlib.h>
#include <X11/Xutil.h>   // XGetPixel, XDestroyImage
#include <poll.h>
#include <time.h>

// --- the host services an X11 editor needs -------------------------------
static int      g_fd = -1;          // fd the plugin asked us to poll
static clap_id  g_timer_id = 0;
static uint32_t g_timer_ms = 0;
static int      g_has_timer = 0;
static int      g_embed = 0;        // only offer these under --embed

static bool host_fd_register(const clap_host_t *h, int fd, clap_posix_fd_flags_t f)
{ (void)h; (void)f; g_fd = fd; printf("      host: registered fd %d\n", fd); return true; }
static bool host_fd_modify(const clap_host_t *h, int fd, clap_posix_fd_flags_t f)
{ (void)h; (void)fd; (void)f; return true; }
static bool host_fd_unregister(const clap_host_t *h, int fd)
{ (void)h; printf("      host: unregistered fd %d\n", fd); if (g_fd == fd) g_fd = -1; return true; }

static const clap_host_posix_fd_support_t g_fd_ext = {
	.register_fd = host_fd_register,
	.modify_fd = host_fd_modify,
	.unregister_fd = host_fd_unregister,
};

static bool host_timer_register(const clap_host_t *h, uint32_t ms, clap_id *id)
{ (void)h; g_timer_ms = ms; *id = ++g_timer_id; g_has_timer = 1;
  printf("      host: registered timer %u ms (id %u)\n", ms, *id); return true; }
static bool host_timer_unregister(const clap_host_t *h, clap_id id)
{ (void)h; printf("      host: unregistered timer id %u\n", id); g_has_timer = 0; return true; }

static const clap_host_timer_support_t g_timer_ext = {
	.register_timer = host_timer_register,
	.unregister_timer = host_timer_unregister,
};

static const void *host_get_extension(const struct clap_host *h, const char *id)
{
	(void)h;
	// Only under --embed. Without it this stays the minimal loader it was, so
	// the earlier S37 measurements remain reproducible: a plugin that needs an
	// extension should say so rather than be handed a stub that hides it.
	if (g_embed && !strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT)) return &g_fd_ext;
	if (g_embed && !strcmp(id, CLAP_EXT_TIMER_SUPPORT))    return &g_timer_ext;
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
	int do_activate = 0, do_gui = 0, do_state = 0, do_embed = 0;
	double embed_secs = 3.0;
	for (int i = 2; i < argc; ++i) {
		if (!strcmp(argv[i], "--activate")) do_activate = 1;
		else if (!strcmp(argv[i], "--gui")) do_gui = 1;
		else if (!strcmp(argv[i], "--state")) do_state = 1;
		else if (!strcmp(argv[i], "--embed")) {
			do_gui = do_embed = g_embed = 1;
			if (i + 1 < argc && argv[i+1][0] != '-') embed_secs = atof(argv[++i]);
		}
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

				int destroyed = 0;
				Window child = 0; unsigned cw = 0, ch = 0;
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

					if (do_embed) {
						Display *dpy = XOpenDisplay(NULL);
						if (!dpy) { printf("      XOpenDisplay FAILED (is DISPLAY set?)\n"); rc = 1; }
						else {
							int scr = DefaultScreen(dpy);
							if (!w) w = 1100; if (!h) h = 600;
							Window parent = XCreateSimpleWindow(dpy, RootWindow(dpy, scr),
							                                    0, 0, w, h, 0, 0, 0);
							XMapWindow(dpy, parent); XSync(dpy, False);
							printf("      host: parent Window 0x%lx (%ux%u)\n",
							       (unsigned long)parent, w, h);

							clap_window_t win = { .api = CLAP_WINDOW_API_X11 };
							win.x11 = (uint64_t)parent;

							if (!gui->set_parent(p, &win)) {
								printf("      gui->set_parent FAILED\n"); rc = 1;
							} else {
								printf("      set_parent OK\n");
								if (gui->show && !gui->show(p))
									printf("      gui->show declined (not fatal)\n");
								else
									printf("      show OK\n");

								// THE PUMP. This is the contract X11DrawingFrame is
								// built around: it runs no loop, so if the host does
								// not poll the fd and tick the timer, nothing paints.
								const clap_plugin_posix_fd_support_t *pfd =
									p->get_extension(p, CLAP_EXT_POSIX_FD_SUPPORT);
								const clap_plugin_timer_support_t *ptm =
									p->get_extension(p, CLAP_EXT_TIMER_SUPPORT);
								printf("      plugin exts: posix_fd=%s timer=%s\n",
								       pfd ? "yes" : "NO", ptm ? "yes" : "NO");

								struct timespec t0, now;
								clock_gettime(CLOCK_MONOTONIC, &t0);
								long fd_events = 0, ticks = 0;
								for (;;) {
									clock_gettime(CLOCK_MONOTONIC, &now);
									double el = (now.tv_sec - t0.tv_sec)
									          + (now.tv_nsec - t0.tv_nsec) / 1e9;
									if (el >= embed_secs) break;

									struct pollfd pf = { .fd = g_fd, .events = POLLIN };
									int n = (g_fd >= 0) ? poll(&pf, 1, 16) : (poll(NULL,0,16), 0);
									if (n > 0 && pfd && pfd->on_fd) {
										pfd->on_fd(p, g_fd, CLAP_POSIX_FD_READ); fd_events++;
									}
									if (g_has_timer && ptm && ptm->on_timer) {
										ptm->on_timer(p, g_timer_id); ticks++;
									}
								}
								printf("      pumped %.1fs: %ld fd events, %ld timer ticks\n",
								       embed_secs, fd_events, ticks);

								// STRUCTURE FIRST: did the plugin actually create and
								// map a child of our window? That is what "embedded"
								// means, and unlike pixels it is unambiguous on a
								// headless server.
								{
									Window r = 0, par = 0, *kids = NULL; unsigned nk = 0;
									child = 0;
									if (XQueryTree(dpy, parent, &r, &par, &kids, &nk) && nk) {
										printf("      parent has %u child window(s):\n", nk);
										for (unsigned k = 0; k < nk; ++k) {
											XWindowAttributes wa;
											if (XGetWindowAttributes(dpy, kids[k], &wa))
												printf("        0x%lx %dx%d at %d,%d map_state=%s\n",
												       (unsigned long)kids[k], wa.width, wa.height,
												       wa.x, wa.y,
												       wa.map_state == IsViewable ? "IsViewable"
												       : wa.map_state == IsUnmapped ? "IsUnmapped"
												                                    : "IsUnviewable");
											if (!child) { child = kids[k]; cw = wa.width; ch = wa.height; }
										}
									} else {
										printf("      parent has NO child window -- not embedded\n");
										rc = 1;
									}
									if (kids) XFree(kids);
								}

								// DID IT ACTUALLY PAINT? "set_parent returned true"
								// is not the same claim. Read the pixels back and
								// count distinct colours: an unpainted window is
								// one flat colour, a drawn rack is not.
								// Sample the PLUGIN's window, not ours -- XGetImage on a
								// parent is not a reliable way to see a child's pixels.
								Window target = child ? child : parent;
								unsigned tw = child ? cw : w, th = child ? ch : h;
								XImage *img = XGetImage(dpy, target, 0, 0, tw, th,
								                        AllPlanes, ZPixmap);
								if (!img) {
									printf("      XGetImage FAILED -- cannot judge paint\n");
								} else {
									unsigned long seen[64]; int nseen = 0;
									for (unsigned y = 0; y < th && nseen < 64; y += 7)
										for (unsigned x = 0; x < tw && nseen < 64; x += 7) {
											unsigned long px = XGetPixel(img, x, y);
											int k = 0;
											for (; k < nseen; ++k) if (seen[k] == px) break;
											if (k == nseen) seen[nseen++] = px;
										}
									printf("      window 0x%lx content: %d distinct colours sampled%s\n",
									       (unsigned long)target, nseen,
									       nseen > 1 ? "  <-- IT PAINTED" : "  <-- flat");
									for (int q = 0; q < nseen && q < 6; ++q)
										printf("        colour[%d] = 0x%06lx\n", q, seen[q] & 0xffffffUL);
									if (nseen <= 1) rc = 1;
									// Dump it, so "it painted" is something a human can check.
									const char *dump = getenv("CLAP_PROBE_PPM");
									if (dump) {
										FILE *f = fopen(dump, "wb");
										if (f) {
											fprintf(f, "P6\n%u %u\n255\n", tw, th);
											for (unsigned y = 0; y < th; ++y)
												for (unsigned x = 0; x < tw; ++x) {
													unsigned long v = XGetPixel(img, x, y);
													unsigned char rgb[3] = {
														(unsigned char)((v >> 16) & 0xff),
														(unsigned char)((v >> 8) & 0xff),
														(unsigned char)(v & 0xff) };
													fwrite(rgb, 1, 3, f);
												}
											fclose(f);
											printf("      wrote %s\n", dump);
										}
									}
									XDestroyImage(img);
								}
							}
						}
						// ORDER MATTERS, and getting it wrong produced a BadWindow
						// on X_DestroyWindow: closing our display frees the parent
						// window, and the plugin then tries to destroy its child of
						// a window that no longer exists. A real host tears the
						// plugin GUI down first, so the probe must too.
						gui->destroy(p);
						destroyed = 1;
						printf("      gui destroyed cleanly\n");
						if (dpy) { XSync(dpy, False); XCloseDisplay(dpy); }
					}

					if (!destroyed) {
						gui->destroy(p);
						printf("      gui destroyed cleanly\n");
					}
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
