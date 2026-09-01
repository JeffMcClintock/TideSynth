/* BACKLOG E78 -- does a hosted Linux CLAP deliver DSP->GUI parameter updates?
 *
 * WHY THIS EXISTS RATHER THAN A DAW. E78's Accept wants a prepared rack in a
 * hosted CLAP with the editor OPEN, and REAPER 7.43 cannot supply that on this
 * box: TrackFX_Show on the CLAP -- float or chain, both tried -- kills the host
 * from inside its own GTK, `gdk_screen_get_root_window: assertion
 * 'GDK_IS_SCREEN (screen)' failed` and three more, so REAPER never reaches
 * guiSetParent with a usable window. That is REAPER's headless-X11 problem and
 * not the plug-in's, and it is not worth chasing to measure a one-line fix.
 *
 * WHAT IT DOES: opens a real X11 window, drives clap_plugin_gui through
 * is_api_supported / create / set_parent / show, runs process() on a REAL
 * second thread while the main thread services the two host extensions the
 * Linux editor requires -- clap_host_timer_support and
 * clap_host_posix_fd_support -- and stops after a wall-clock window.
 *
 * THE POINT IS THE TIMER. On Linux the CLAP editor registers a 16 ms host timer
 * in guiSetParent (Editor_CLAP.cpp) and unregisters it in guiDestroy, and that
 * timer is the ONLY UI-thread tick a hosted plug-in gets. Everything
 * Controller_CLAP::onTimer does -- BOTH directions of the parameter channel --
 * hangs off it. So a host that does not implement timer_support measures
 * nothing here, and a plug-in that is not pumped from it does nothing.
 *
 * The evidence is the plug-in's own RACK_ADAPTOR_TRACE counters on stderr, read
 * the same way the VST3 harness reads them; this probe reports the SETUP facts
 * and how many times it ticked, so a run with zero updates can be told apart
 * from a run that never got a window.
 *
 * Build (CLAP headers are header-only):
 *   cc -std=c11 -I<clap-src>/include tests/e78_clap_gui_probe.c -ldl -lX11 -lpthread \
 *      -o e78_clap_gui_probe
 *
 * Run (needs DISPLAY):
 *   ./e78_clap_gui_probe <path/to/TIDE-Rack.clap> <preset.xml> [seconds]
 *
 * Exit codes, deliberately distinct -- "the harness could not set up" and "the
 * plug-in did nothing" must not look alike (the E62 lesson):
 *   0  the editor was created, parented and shown, and the timer ticked
 *   1  the plug-in refused something it should not have (gui or state)
 *   2  harness/setup failure -- no display, no dlopen, no factory, no gui ext
 */
#define _POSIX_C_SOURCE 200809L

#include <dlfcn.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>

#include <clap/clap.h>

/* ---- the one timer and the one fd this host needs to remember ------------- */
static clap_id  g_timerId       = CLAP_INVALID_ID;
static uint32_t g_timerPeriodMs = 0;
static int      g_fd            = -1;
static int      g_timerTicks    = 0;
static int      g_fdEvents      = 0;

static const clap_plugin_t* g_plugin = NULL;

static int64_t nowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ---- host extensions ------------------------------------------------------ */
static bool timerRegister(const clap_host_t* h, uint32_t periodMs, clap_id* id)
{
    (void)h;
    if (g_timerId != CLAP_INVALID_ID)
        return false;            /* one is all the editor asks for */
    g_timerId = 1;
    g_timerPeriodMs = periodMs < 1 ? 1 : periodMs;
    *id = g_timerId;
    printf("host: register_timer(%u ms) -> id %u\n", periodMs, (unsigned)g_timerId);
    return true;
}
static bool timerUnregister(const clap_host_t* h, clap_id id)
{
    (void)h;
    if (id != g_timerId) return false;
    printf("host: unregister_timer(%u)\n", (unsigned)id);
    g_timerId = CLAP_INVALID_ID;
    return true;
}
static const clap_host_timer_support_t g_timerSupport = {
    .register_timer = timerRegister, .unregister_timer = timerUnregister,
};

static bool fdRegister(const clap_host_t* h, int fd, clap_posix_fd_flags_t flags)
{
    (void)h; (void)flags;
    g_fd = fd;
    printf("host: register_fd(%d)\n", fd);
    return true;
}
static bool fdModify(const clap_host_t* h, int fd, clap_posix_fd_flags_t flags)
{ (void)h; (void)fd; (void)flags; return true; }
static bool fdUnregister(const clap_host_t* h, int fd)
{ (void)h; if (fd == g_fd) g_fd = -1; return true; }
static const clap_host_posix_fd_support_t g_fdSupport = {
    .register_fd = fdRegister, .modify_fd = fdModify, .unregister_fd = fdUnregister,
};

static void guiResizeHintsChanged(const clap_host_t* h) { (void)h; }
static bool guiRequestResize(const clap_host_t* h, uint32_t w, uint32_t ht)
{ (void)h; (void)w; (void)ht; return true; }
static bool guiRequestShow(const clap_host_t* h) { (void)h; return true; }
static bool guiRequestHide(const clap_host_t* h) { (void)h; return true; }
static void guiClosed(const clap_host_t* h, bool wasDestroyed)
{ (void)h; (void)wasDestroyed; }
static const clap_host_gui_t g_hostGui = {
    .resize_hints_changed = guiResizeHintsChanged,
    .request_resize = guiRequestResize,
    .request_show = guiRequestShow,
    .request_hide = guiRequestHide,
    .closed = guiClosed,
};

static const void* hostGetExtension(const clap_host_t* h, const char* id)
{
    (void)h;
    if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT))     return &g_timerSupport;
    if (!strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT))  return &g_fdSupport;
    if (!strcmp(id, CLAP_EXT_GUI))               return &g_hostGui;
    return NULL;
}
static void hostRequestRestart(const clap_host_t* h)  { (void)h; }
static void hostRequestProcess(const clap_host_t* h)  { (void)h; }
static void hostRequestCallback(const clap_host_t* h) { (void)h; }

/* ---- the preset, read through a short-count stream ------------------------ */
typedef struct { const char* data; size_t size, pos; clap_istream_t api; } InStream;
static int64_t inRead(const clap_istream_t* s, void* buffer, uint64_t size)
{
    InStream* self = (InStream*)s->ctx;
    size_t left = self->size - self->pos;
    size_t n = size < left ? (size_t)size : left;
    if (n > 4096) n = 4096;              /* a real host returns short counts */
    memcpy(buffer, self->data + self->pos, n);
    self->pos += n;
    return (int64_t)n;
}

/* ---- the audio thread ----------------------------------------------------- */
static volatile int g_run = 1;
static double  g_rate  = 44100.0;
static uint32_t g_block = 512;

static uint32_t evSize(const clap_input_events_t* l) { (void)l; return 0; }
static const clap_event_header_t* evGet(const clap_input_events_t* l, uint32_t i)
{ (void)l; (void)i; return NULL; }
static bool evPush(const clap_output_events_t* l, const clap_event_header_t* e)
{ (void)l; (void)e; return true; }

static void* audioThread(void* arg)
{
    (void)arg;
    float left[4096], right[4096];
    float* chans[2] = { left, right };
    clap_audio_buffer_t out = { .data32 = chans, .data64 = NULL,
        .channel_count = 2, .latency = 0, .constant_mask = 0 };
    clap_input_events_t  inEv  = { .ctx = NULL, .size = evSize, .get = evGet };
    clap_output_events_t outEv = { .ctx = NULL, .try_push = evPush };

    uint64_t steady = 0;
    /* Paced to real time, because the plug-in's display-state rate is derived
     * from the SAMPLE rate: running flat out would make 30 Hz of picture into
     * whatever the CPU manages, and the counters would not be comparable with
     * the VST3 harness's. */
    const long blockNs = (long)(1e9 * (double)g_block / g_rate);
    while (g_run)
    {
        memset(left, 0, sizeof left);
        memset(right, 0, sizeof right);
        clap_process_t proc = {
            .steady_time = (int64_t)steady, .frames_count = g_block,
            .transport = NULL,
            .audio_inputs = NULL, .audio_inputs_count = 0,
            .audio_outputs = &out, .audio_outputs_count = 1,
            .in_events = &inEv, .out_events = &outEv,
        };
        g_plugin->process(g_plugin, &proc);
        steady += g_block;
        struct timespec ts = { .tv_sec = 0, .tv_nsec = blockNs };
        nanosleep(&ts, NULL);
    }
    return NULL;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <TIDE-Rack.clap> <preset.xml> [seconds]\n", argv[0]);
        return 2;
    }
    const char* clapPath   = argv[1];
    const char* presetPath = argv[2];
    const int   seconds    = argc > 3 ? atoi(argv[3]) : 30;
    /* Block size and rate are overridable because they are the two things a
     * host varies that this probe otherwise fixes -- and "the probe is not a
     * DAW" is the first objection to any negative result it produces. */
    if (getenv("E78_BLOCK")) g_block = (uint32_t)atoi(getenv("E78_BLOCK"));
    if (getenv("E78_RATE"))  g_rate  = atof(getenv("E78_RATE"));
    if (g_block < 1 || g_block > 4096) { fprintf(stderr, "SETUP: bad E78_BLOCK\n"); return 2; }

    /* the preset */
    FILE* pf = fopen(presetPath, "rb");
    if (!pf) { fprintf(stderr, "SETUP: cannot read %s\n", presetPath); return 2; }
    fseek(pf, 0, SEEK_END); long psz = ftell(pf); fseek(pf, 0, SEEK_SET);
    char* preset = malloc((size_t)psz + 1);
    if (fread(preset, 1, (size_t)psz, pf) != (size_t)psz)
    { fprintf(stderr, "SETUP: short read of %s\n", presetPath); return 2; }
    fclose(pf);
    printf("preset           %s (%ld bytes)\n", presetPath, psz);

    /* a real X11 window */
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "SETUP: XOpenDisplay failed (DISPLAY=%s)\n",
                        getenv("DISPLAY") ? getenv("DISPLAY") : "(unset)"); return 2; }
    int screen = DefaultScreen(dpy);
    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, 1200, 800, 0,
                                     BlackPixel(dpy, screen), WhitePixel(dpy, screen));
    XSelectInput(dpy, win, StructureNotifyMask | ExposureMask);
    XMapWindow(dpy, win);
    XFlush(dpy);
    printf("host window      0x%lx on %s\n", (unsigned long)win, XDisplayString(dpy));

    void* lib = dlopen(clapPath, RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "SETUP: dlopen: %s\n", dlerror()); return 2; }
    const clap_plugin_entry_t* entry =
        (const clap_plugin_entry_t*)dlsym(lib, "clap_entry");
    if (!entry || !entry->init(clapPath))
    { fprintf(stderr, "SETUP: no clap_entry / init failed\n"); return 2; }

    const clap_plugin_factory_t* factory =
        (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    if (!factory || factory->get_plugin_count(factory) == 0)
    { fprintf(stderr, "SETUP: no factory\n"); return 2; }
    const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, 0);
    printf("plugin           %s (%s)\n", desc->name ? desc->name : "?", desc->id);

    clap_host_t host = {
        .clap_version = CLAP_VERSION, .host_data = NULL,
        .name = "e78_clap_gui_probe", .vendor = "TIDE", .url = "", .version = "1",
        .get_extension = hostGetExtension, .request_restart = hostRequestRestart,
        .request_process = hostRequestProcess, .request_callback = hostRequestCallback,
    };

    const clap_plugin_t* plug = factory->create_plugin(factory, &host, desc->id);
    if (!plug || !plug->init(plug))
    { fprintf(stderr, "SETUP: create/init failed\n"); return 2; }
    g_plugin = plug;

    /* the prepared rack, before the GUI exists -- the same order a DAW uses */
    const clap_plugin_state_t* state =
        (const clap_plugin_state_t*)plug->get_extension(plug, CLAP_EXT_STATE);
    if (!state) { fprintf(stderr, "SETUP: no %s\n", CLAP_EXT_STATE); return 2; }
    InStream in = { .data = preset, .size = (size_t)psz, .pos = 0 };
    in.api.ctx = &in; in.api.read = inRead;
    const bool loaded = state->load(plug, &in.api);
    printf("state->load      %s, %zu of %ld bytes\n", loaded ? "true" : "false",
           in.pos, psz);
    if (!loaded) { fprintf(stderr, "the plug-in refused the preset\n"); return 1; }

    /* the GUI */
    const clap_plugin_gui_t* gui =
        (const clap_plugin_gui_t*)plug->get_extension(plug, CLAP_EXT_GUI);
    if (!gui) { fprintf(stderr, "SETUP: no %s\n", CLAP_EXT_GUI); return 2; }

    const bool apiOk = gui->is_api_supported(plug, CLAP_WINDOW_API_X11, false);
    printf("is_api_supported %s (x11, embedded)\n", apiOk ? "true" : "false");
    if (!apiOk) { fprintf(stderr, "the plug-in declines x11\n"); return 1; }

    if (!gui->create(plug, CLAP_WINDOW_API_X11, false))
    { fprintf(stderr, "gui->create failed\n"); return 1; }
    printf("gui->create      true\n");

    clap_window_t w = { .api = CLAP_WINDOW_API_X11 };
    w.x11 = (uint64_t)win;
    if (!gui->set_parent(plug, &w))
    { fprintf(stderr, "gui->set_parent failed\n"); return 1; }
    printf("gui->set_parent  true\n");
    if (!gui->show(plug))
        printf("gui->show        false (continuing; some editors have nothing to show)\n");
    else
        printf("gui->show        true\n");

    uint32_t gw = 0, gh = 0;
    if (gui->get_size(plug, &gw, &gh))
        printf("gui->get_size    %ux%u\n", gw, gh);

    if (g_timerId == CLAP_INVALID_ID)
        fprintf(stderr, "NOTE: the plug-in registered NO timer -- nothing here can tick it\n");

    /* audio */
    if (!plug->activate(plug, g_rate, g_block, g_block))
    { fprintf(stderr, "SETUP: activate failed\n"); return 2; }
    if (!plug->start_processing(plug))
    { fprintf(stderr, "SETUP: start_processing failed\n"); return 2; }
    pthread_t th;
    pthread_create(&th, NULL, audioThread, NULL);
    printf("processing       %.0f Hz, %u-frame blocks, for %d s\n", g_rate, g_block, seconds);

    const clap_plugin_timer_support_t* pluginTimer =
        (const clap_plugin_timer_support_t*)plug->get_extension(plug, CLAP_EXT_TIMER_SUPPORT);
    const clap_plugin_posix_fd_support_t* pluginFd =
        (const clap_plugin_posix_fd_support_t*)plug->get_extension(plug, CLAP_EXT_POSIX_FD_SUPPORT);

    const int64_t t0 = nowMs();
    int64_t nextTick = t0;
    while (nowMs() - t0 < (int64_t)seconds * 1000)
    {
        struct pollfd pfd = { .fd = g_fd, .events = POLLIN };
        int timeout = (int)(nextTick - nowMs());
        if (timeout < 0) timeout = 0;
        if (g_fd >= 0 && poll(&pfd, 1, timeout) > 0 && pluginFd)
        {
            pluginFd->on_fd(plug, g_fd, CLAP_POSIX_FD_READ);
            ++g_fdEvents;
        }
        else if (g_fd < 0)
        {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
            nanosleep(&ts, NULL);
        }
        if (nowMs() >= nextTick)
        {
            if (pluginTimer && g_timerId != CLAP_INVALID_ID)
            {
                pluginTimer->on_timer(plug, g_timerId);
                ++g_timerTicks;
            }
            nextTick = nowMs() + (int64_t)(g_timerPeriodMs ? g_timerPeriodMs : 16);
        }
    }

    g_run = 0;
    pthread_join(th, NULL);
    plug->stop_processing(plug);
    plug->deactivate(plug);

    printf("timer ticks      %d over %d s\n", g_timerTicks, seconds);
    printf("fd events        %d\n", g_fdEvents);

    gui->destroy(plug);
    plug->destroy(plug);
    entry->deinit();
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    if (g_timerTicks == 0)
    {
        printf("VERDICT          SETUP FAIL -- the editor was never ticked, so this run "
               "says nothing about the parameter channel\n");
        return 2;
    }
    printf("VERDICT          the editor ran and was ticked %d times; read the plug-in's "
           "own RACK_ADAPTOR_TRACE counters on stderr for the result\n", g_timerTicks);
    return 0;
}
