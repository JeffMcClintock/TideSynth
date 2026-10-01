/* BACKLOG E80 -- does the 65,548-byte display-state blob enter TIDE's own
 * DSP->UI queue on a hosted CLAP, or does it not?
 *
 * E80 was filed from Linux and its central number is this: TIDE's own
 * `TIDE: instance #N feedback send #M (B bytes)` counter -- which lives INSIDE
 * the plug-in, upstream of any wrapper -- never packed more than 200 bytes on
 * CLAP there, against 65,673 repeatedly on VST3 and in the standalone on the
 * same tree. The row's step one is, in its own words, "a second opinion, not a
 * fix". This is that second opinion, on Windows.
 *
 * WHY THIS NEEDS NO EDITOR, NO WINDOW AND NO DAW -- which is the whole reason
 * it exists rather than a second REAPER session. Both counters E80 compares are
 * raised on the AUDIO thread, inside process(), with nothing gating them on an
 * editor:
 *
 *   - `RackProcessor: '<slug>' display-state capture #N (B bytes)` is raised in
 *     RackAdaptor.h's sendDisplayState(), called unconditionally from the rack
 *     module's subProcess().
 *   - `TIDE: instance #N feedback send #M (B bytes, H held back)` is raised in
 *     SynthEditSem/SynthEdit.cpp's drainRackFeedback(), called unconditionally
 *     at the end of TIDE's own subProcess().
 *
 * So the pair "the DSP captured 65,548 bytes" / "the queue carried B" is fully
 * observable from a bare host that calls process() and reads stderr. What an
 * editor would add is the OTHER end of the channel (`RackEditor: display-state
 * update #N arrived`), and that end is not what E80's number is about.
 *
 * THIS IS THE FIRST BARE CLAP HOST IN THIS REPO THAT BUILDS ON WINDOWS.
 * tests/e69_clap_state_probe.c, tests/e78_clap_gui_probe.c and
 * tests/e79_clap_headless_probe.c are all `#include <dlfcn.h>` and so are
 * mac/linux only; the windows box had no bare-host instrument at all. The host
 * stub, the istream shim and the note-on event plumbing below are deliberately
 * the same shape as e79's, so that a plug-in change which breaks one is visible
 * in the other.
 *
 *   --pump      (default) run the host's main-thread message loop between
 *               blocks -- PeekMessage/DispatchMessage on Windows,
 *               CFRunLoopRunInMode on macOS. This is what a real host's main
 *               thread does, and it is what feeds gmpi's SetTimer-backed
 *               TimerClient, which is what Controller_CLAP ticks on.
 *   --no-pump   never run it. CONTROL: it starves that timer. If both arms give
 *               the same feedback-send figures, the figure is not a pumping
 *               artefact -- which is the one thing a single-armed probe here
 *               could not tell you, and the trap e79's two arms were built for.
 *   --no-preset NEGATIVE CONTROL: never call state->load, so the plug-in runs
 *               its bundled default rack. That rack has no VCV Scope in it, so
 *               `display-state capture` should not appear at all -- which is
 *               what says the lines in the other arms came from the document
 *               under test rather than from anything the plug-in does anyway.
 *   --editor    CREATE THE PLUG-IN'S EDITOR through clap.gui and leave it open
 *               for the whole render. This is the arm E80's row asks for by
 *               name, and it is the ONE VARIABLE the 2026-09-09 windows run
 *               could not move: every figure it published was measured with no
 *               editor in existence, while E80's linux measurement had one.
 *               With it, the FAR end of the channel becomes observable too --
 *               `RackEditor: display-state update #N arrived (B bytes)`, which
 *               is literally the row's Accept.
 *
 *               THE PARENT WINDOW IS DELIBERATELY INVISIBLE AND OFF-SCREEN.
 *               It is a WS_POPUP that is never given WS_VISIBLE and is placed
 *               at (-32000,-32000), so the editor is embedded, ticking and
 *               rendering, and NOTHING APPEARS ON THE DESKTOP. That is not
 *               tidiness: this box's scheduled runs fire whether or not the
 *               developer is working at it (2026-09-09 found two Visual Studio
 *               instances open mid-edit), and an editor arm that stole focus
 *               would be unrunnable on exactly the days it is scheduled. An
 *               unowned invisible parent cannot take focus and cannot raise
 *               itself over anyone's work.
 *
 *               The host stub gains a clap.gui HOST extension in this arm and
 *               ONLY in this arm, so that --no-editor is byte-identical to the
 *               host the 2026-09-09 figures were read from. Say so when you
 *               quote the two side by side: the arms differ in the editor AND
 *               in that one host extension, and no real host offers the second
 *               without the first.
 *
 * Build (Windows, from a VS x64 developer prompt or with cl on PATH):
 *   cl /std:c11 /nologo /I build-e19win\_deps\clap-src\include \
 *      tests\e80_clap_feedback_probe.c /Fe:e80probe.exe
 *
 * Build (macOS/Linux):
 *   cc -std=c11 -I build/_deps/clap-src/include tests/e80_clap_feedback_probe.c \
 *      -ldl -o e80probe            # add -framework CoreFoundation on macOS
 *
 * Run, and note that the EVIDENCE IS ON STDERR, written by the plug-in:
 *   ./e80probe <path-to.clap> <preset.xml> --blocks 800 2> trace.err
 *   grep -oE "feedback send #[0-9]+ \([0-9]+ bytes" trace.err | sort -u
 *
 * The preset is the OUTER <Preset> element, which is what
 * scripts/decode_rpp.py --preset-out writes and what tests/fixtures/*.xml are.
 * The inner <Document> alone is NOT restorable -- the wrapper wants the
 * <Preset> wrapper.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clap/clap.h>

#if defined(_WIN32)
  #include <windows.h>
  #define TIDE_DLOPEN(p)      ((void *)LoadLibraryA(p))
  #define TIDE_DLSYM(h, s)    ((void *)GetProcAddress((HMODULE)(h), (s)))
#else
  #include <dlfcn.h>
  #define TIDE_DLOPEN(p)      dlopen((p), RTLD_LOCAL | RTLD_NOW)
  #define TIDE_DLSYM(h, s)    dlsym((h), (s))
#endif

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
 * stub as tests/e79_clap_headless_probe.c, and deliberately so. */
static const void *host_get_extension(const clap_host_t *h, const char *id);
static void host_noop(const clap_host_t *h) { (void)h; }

/* --editor only. A real host always offers clap.gui; a bare one that never
 * creates an editor has no reason to, and the 2026-09-09 figures were read from
 * a stub that returned NULL for everything. So this is wired in behind the flag
 * rather than unconditionally -- the control arm must stay the SAME HOST those
 * numbers came from, or the comparison this probe exists to make is confounded
 * by the instrument. */
static int wantEditor = 0;

static void host_gui_resize_hints_changed(const clap_host_t *h) { (void)h; }
static bool host_gui_request_resize(const clap_host_t *h, uint32_t w, uint32_t h_)
{
    /* Accepted, and nothing is done with it: the parent is invisible, so its
     * size is not observable and resizing it would only hide a mismatch. */
    (void)h; (void)w; (void)h_;
    return true;
}
static bool host_gui_request_show(const clap_host_t *h) { (void)h; return false; }
static bool host_gui_request_hide(const clap_host_t *h) { (void)h; return false; }
static void host_gui_closed(const clap_host_t *h, bool was_destroyed)
{
    (void)h;
    fprintf(stderr, "e80probe: host.gui.closed(was_destroyed=%d)\n", (int)was_destroyed);
}
static const clap_host_gui_t host_gui = {
    host_gui_resize_hints_changed,
    host_gui_request_resize,
    host_gui_request_show,
    host_gui_request_hide,
    host_gui_closed,
};

static const void *host_get_extension(const clap_host_t *h, const char *id)
{
    (void)h;
    if (wantEditor && id && !strcmp(id, CLAP_EXT_GUI)) return &host_gui;
    return NULL;
}

static clap_host_t host = {
    CLAP_VERSION_INIT,
    NULL,
    "tide-e80-probe",
    "TIDE Synth",
    "",
    "1.0",
    host_get_extension,
    host_noop,
    host_noop,
    host_noop,
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
    char *b = (char *)malloc((size_t)n + 1);
    if (!b) { fclose(f); return NULL; }
    size_t rd = fread(b, 1, (size_t)n, f);
    fclose(f);
    b[rd] = 0;
    *out_len = rd;
    return b;
}

/* Let the host's main thread run, the way a real host's does between audio
 * callbacks. THE PLATFORMS DIFFER IN WHAT THAT MEANS, and the difference is the
 * reason this is a function rather than a sleep: gmpi's TimerClient is backed by
 * SetTimer on Windows and by a CFRunLoopTimer on macOS, so "give the main thread
 * a slice" is a message pump in one place and a run loop in the other. On Linux
 * neither exists in a bare host, which is exactly what E79 recorded. */
static void pump_main_thread(double seconds)
{
#if defined(_WIN32)
    /* WM_TIMER is a synthesised, lowest-priority message: PeekMessage only
     * reports it when the queue is otherwise empty, which is precisely the
     * condition a bare host is always in. Loop for the requested slice rather
     * than draining once, so a 15 ms tick inside a 1 ms slice is not missed by
     * construction. */
    const DWORD until = GetTickCount() + (DWORD)(seconds * 1000.0);
    for (;;) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (GetTickCount() >= until) break;
        Sleep(1);
    }
#elif defined(__APPLE__)
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, seconds, false);
#else
    (void)seconds;
#endif
}

/* ---- events: a single note-on, so the rack's envelopes actually open ------ */

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

/* ---- the editor arm ------------------------------------------------------- *
 *
 * An embedded CLAP editor needs a parent window handle and a main thread that
 * pumps messages. Both exist here; what deliberately does NOT exist is any way
 * for the result to appear on screen. The parent is a WS_POPUP created WITHOUT
 * WS_VISIBLE at (-32000,-32000) with no owner, so:
 *
 *   - it is never shown, so it cannot raise itself over the developer's work;
 *   - it has no owner and no WS_EX_APPWINDOW, so it gets no taskbar button;
 *   - the plug-in's own window is created as its CHILD, and a child of a window
 *     that was never shown is not shown either, whatever the child does with
 *     ShowWindow.
 *
 * That is not tidiness. This box's scheduled runs fire whether or not the
 * developer is working at it -- 2026-09-09 found two Visual Studio instances
 * open mid-edit -- so an editor arm that stole focus would be unrunnable on
 * exactly the days it is scheduled.
 *
 * The editor still RUNS: WM_TIMER is delivered to an invisible window exactly
 * as it is to a visible one, and gmpi's TimerClient is SetTimer-backed on
 * Windows. What an invisible window does not get is WM_PAINT -- so a figure
 * that depends on the editor having actually PAINTED is NOT observable here and
 * must be reported as such. E80's far-end counter is not one of those:
 * RackEditor.h raises `display-state update #N arrived` from the pin-set path,
 * not from render(), and `RackEditor: render #N` is a separate line you can
 * check for independently to see which of the two you are looking at.
 */
#if defined(_WIN32)
static const char *kProbeWndClass = "TideE80ProbeParent";

static LRESULT CALLBACK probe_wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    return DefWindowProcA(h, m, w, l);
}

static HWND probe_make_hidden_parent(uint32_t w, uint32_t h)
{
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc   = probe_wndproc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = kProbeWndClass;
    RegisterClassA(&wc);          /* a repeat registration fails harmlessly */

    /* No WS_VISIBLE, no owner, off-screen origin. None of that is cosmetic. */
    return CreateWindowExA(
        0, kProbeWndClass, "tide e80 probe (never shown)",
        WS_POPUP | WS_CLIPCHILDREN,
        -32000, -32000, (int)w, (int)h,
        NULL, NULL, GetModuleHandleA(NULL), NULL);
}
#endif

int main(int argc, char **argv)
{
    const char *bundle = NULL, *presetPath = NULL;
    const clap_plugin_gui_t *gui = NULL;
    int guiCreated = 0, guiParented = 0;
#if defined(_WIN32)
    HWND parentWnd = NULL;
#endif
    int usePump = 1, blocks = 800, loadPreset = 1;
    const uint32_t blockSize = 512;
    const double sampleRate = 44100.0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--pump"))            usePump = 1;
        else if (!strcmp(argv[i], "--no-pump"))    usePump = 0;
        else if (!strcmp(argv[i], "--no-preset"))  loadPreset = 0;
        else if (!strcmp(argv[i], "--editor"))     wantEditor = 1;
        else if (!strcmp(argv[i], "--no-editor"))  wantEditor = 0;
        else if (!strcmp(argv[i], "--blocks") && i + 1 < argc) blocks = atoi(argv[++i]);
        else if (!bundle)     bundle = argv[i];
        else if (!presetPath) presetPath = argv[i];
    }
    if (!bundle || (!presetPath && loadPreset)) {
        fprintf(stderr,
            "usage: %s <path-to.clap> <preset.xml> [--pump|--no-pump] [--no-preset]\n"
            "       [--editor|--no-editor] [--blocks N]\n",
            argv[0]);
        return 2;
    }

    printf("e80_clap_feedback_probe: arm = %s%s, %d blocks of %u at %.0f Hz, %s\n\n",
           usePump ? "--pump (a host main thread runs)"
                   : "--no-pump (starves the controller's timer)",
           loadPreset ? "" : " + --no-preset (NEGATIVE CONTROL: the bundled default rack)",
           blocks, blockSize, sampleRate,
           wantEditor ? "EDITOR CREATED (embedded in an invisible off-screen parent)"
                      : "editor NEVER created");

    /* A .clap is a bundle DIRECTORY on macOS and a plain shared library
     * everywhere else -- on Windows it is a DLL whose extension happens to be
     * .clap, which LoadLibrary is perfectly happy with. */
    char sopath[2048];
#if defined(__APPLE__)
    {
        const char *base = strrchr(bundle, '/');
        base = base ? base + 1 : bundle;
        char name[512];
        snprintf(name, sizeof name, "%s", base);
        char *dot = strrchr(name, '.');
        if (dot) *dot = '\0';
        snprintf(sopath, sizeof sopath, "%s/Contents/MacOS/%s", bundle, name);
    }
#else
    snprintf(sopath, sizeof sopath, "%s", bundle);
#endif

    void *lib = TIDE_DLOPEN(sopath);
    check("the plug-in binary loads", lib != NULL);
    if (!lib) {
#if defined(_WIN32)
        fprintf(stderr, "  LoadLibrary(%s) failed, GetLastError=%lu\n",
                sopath, (unsigned long)GetLastError());
#else
        fprintf(stderr, "  dlerror: %s\n", dlerror());
#endif
        return 1;
    }

    const clap_plugin_entry_t *entry =
        (const clap_plugin_entry_t *)TIDE_DLSYM(lib, "clap_entry");
    check("clap_entry is exported", entry != NULL);
    if (!entry) return 1;
    check("clap_entry->init succeeds", entry->init(bundle));

    const clap_plugin_factory_t *factory =
        (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    check("the plugin factory is offered", factory != NULL);
    if (!factory) return 1;

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
    check("descriptor 0 exists", desc != NULL);
    if (!desc) return 1;
    printf("      plug-in id: %s\n", desc->id ? desc->id : "(null)");

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
        clap_istream_t is;
        is.ctx = &src;
        is.read = source_read;
        check("state->load accepts the preset", state->load(plug, &is));
    } else {
        printf("      NO state->load at all (negative control: the DEFAULT rack)\n");
    }

    /* THE HANDOVER WINDOW. A real host restores state and then goes back to its
     * event loop before the first audio callback. The control arm skips it. */
    if (usePump) {
        printf("      pumping the main thread for 0.5 s (the host's restore->play gap)\n");
        pump_main_thread(0.5);
    } else {
        printf("      NOT pumping the main thread (control arm)\n");
    }

    /* ---- --editor: bring the editor up BEFORE activate ---------------------
     *
     * Order matters and this is the host-like one: a DAW restores state, the
     * user has the editor open, and then the transport rolls. Creating it after
     * activate would also work, but it would make "the editor missed the first
     * N blocks" a live explanation for any shortfall, and this arm exists to
     * REMOVE explanations, not add them. */
    if (wantEditor) {
        gui = (const clap_plugin_gui_t *)plug->get_extension(plug, CLAP_EXT_GUI);
        check("the plugin offers clap.gui", gui != NULL);
    }
#if defined(_WIN32)
    if (gui) {
        uint32_t w = 0, h = 0;

        check("clap.gui supports the win32 api, embedded",
              gui->is_api_supported(plug, CLAP_WINDOW_API_WIN32, false));

        guiCreated = gui->create(plug, CLAP_WINDOW_API_WIN32, false);
        check("gui->create succeeds", guiCreated);

        if (guiCreated) {
            /* set_scale is optional and a plug-in may decline it; a false here
             * is not a failure, so it is reported rather than checked. */
            const bool scaled = gui->set_scale ? gui->set_scale(plug, 1.0) : false;
            printf("      gui->set_scale(1.0) -> %s\n", scaled ? "true" : "declined");

            if (!gui->get_size(plug, &w, &h) || w == 0 || h == 0) {
                w = 1024; h = 768;
                printf("      gui->get_size declined; using %ux%u for the parent\n", w, h);
            } else {
                printf("      gui->get_size -> %ux%u\n", w, h);
            }

            parentWnd = probe_make_hidden_parent(w, h);
            check("an invisible off-screen parent window was created", parentWnd != NULL);
            printf("      parent HWND is %s (IsWindowVisible=%d) -- nothing appears on screen\n",
                   parentWnd ? "valid" : "NULL",
                   parentWnd ? (int)IsWindowVisible(parentWnd) : -1);

            if (parentWnd) {
                clap_window_t cw;
                memset(&cw, 0, sizeof cw);
                cw.api   = CLAP_WINDOW_API_WIN32;
                cw.win32 = (clap_hwnd)parentWnd;
                guiParented = gui->set_parent(plug, &cw);
                check("gui->set_parent succeeds", guiParented);

                if (guiParented) {
                    check("gui->show succeeds", gui->show(plug));
                    /* Give the editor a slice to build itself and bind to the
                     * controller before the first process() call. */
                    pump_main_thread(0.5);
                    printf("      editor is up; parent IsWindowVisible=%d, child count follows\n",
                           (int)IsWindowVisible(parentWnd));
                    {
                        HWND child = GetWindow(parentWnd, GW_CHILD);
                        int n = 0;
                        while (child) { ++n; child = GetWindow(child, GW_HWNDNEXT); }
                        printf("      the plug-in created %d child window(s) under it\n", n);
                    }
                }
            }
        }
    }
#else
    if (wantEditor)
        printf("      --editor is win32-only in this probe; mac/linux have "
               "tests/e78_clap_gui_probe.c\n");
#endif

    check("activate succeeds", plug->activate(plug, sampleRate, 1, blockSize));
    check("start_processing succeeds", plug->start_processing(plug));

    float *L = (float *)calloc(blockSize, sizeof(float));
    float *R = (float *)calloc(blockSize, sizeof(float));
    float *chans[2] = { L, R };
    clap_audio_buffer_t outBus;
    memset(&outBus, 0, sizeof outBus);
    outBus.data32 = chans;
    outBus.data64 = NULL;
    outBus.channel_count = 2;
    outBus.latency = 0;
    outBus.constant_mask = 0;

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

    clap_input_events_t  inEv;
    inEv.ctx = &ev; inEv.size = in_size; inEv.get = in_get;
    clap_output_events_t outEv;
    outEv.ctx = NULL; outEv.try_push = out_try_push;

    double peak = 0.0, sumsq = 0.0;
    uint64_t frames = 0;
    int processFailures = 0;

    for (int b = 0; b < blocks; ++b) {
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

        if (usePump) pump_main_thread(0.001);
    }

    check("no block returned CLAP_PROCESS_ERROR", processFailures == 0);

    /* The audio figures are NOT this probe's subject -- they are here because
     * they cost nothing and because silence would tell you the document never
     * reached the DSP, which would make every feedback number meaningless. */
    const double rms = sqrt(sumsq / (double)(frames * 2));
    printf("\n      %.3f s rendered: peak %.6f (%.1f dBFS), rms %.1f dBFS\n",
           (double)frames / sampleRate, peak,
           peak > 0.0 ? 20.0 * log10(peak) : -1000.0,
           rms  > 0.0 ? 20.0 * log10(rms)  : -1000.0);

    plug->stop_processing(plug);
    plug->deactivate(plug);

    if (gui && guiCreated) {
        if (guiParented) gui->hide(plug);
        gui->destroy(plug);
    }
#if defined(_WIN32)
    if (parentWnd) DestroyWindow(parentWnd);
#endif

    plug->destroy(plug);
    entry->deinit();
    free(L); free(R); free(preset);

    printf("\n%s -- the numbers that matter are on STDERR, from the plug-in:\n"
           "  RackProcessor: '<slug>' display-state capture #N (B bytes)   <- the DSP captured it\n"
           "  TIDE: instance #N feedback send #M (B bytes, H held back)    <- what the queue carried\n"
           "%s",
           failures ? "SOME CHECKS FAILED" : "all probe checks passed",
           wantEditor
             ? "  RackEditor: display-state update #N arrived (B bytes)       <- E80's Accept, the FAR end\n"
               "  RackEditor: light N update #M value V                       <- the small-payload control\n"
             : "");

    return failures ? 1 : 0;
}
