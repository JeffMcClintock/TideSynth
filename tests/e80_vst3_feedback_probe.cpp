/* BACKLOG E80 -- does the 65,548-byte display-state blob enter TIDE's own
 * DSP->UI queue on a hosted VST3, or does it not?
 *
 * THIS IS THE ARM E80's ROW ASKS FOR BY NAME, and it is the one that decides
 * whether the row is about CLAP at all.
 *
 * The whole row rests on an asymmetry: TIDE's own
 * `TIDE: instance #N feedback send #M (B bytes)` counter -- which lives INSIDE
 * the plug-in, upstream of every wrapper -- was reported as never exceeding
 * ~200 bytes on CLAP, against "65,673 repeatedly on VST3 and in the
 * standalone". Two things have since happened to that pair of numbers:
 *
 *   1. 2026-09-09 (windows) showed the CLAP side of it reproduces on Windows
 *      in a bare host, and added TIDE_FEEDBACK_TRACE_EVERY because
 *      `feedback send` only PRINTS #0, #1, #2 and every 100th. A blob sent
 *      once among ~570 sends has about a 2% chance of landing on a sample, so
 *      a figure read off that cadence cannot answer "did this ever happen".
 *   2. 2026-09-10 (windows) showed the editor is not the variable on CLAP:
 *      569 sends / max 337 bytes, identical with an editor, without one, and
 *      without a message pump.
 *
 * THE VST3 FIGURE HAS NEVER BEEN RE-READ WITH THE FULL TRACE. It was measured
 * in REAPER, with an editor, on the old 1-in-100 cadence -- so it differs from
 * every CLAP arm in three variables at once, and the cadence problem applies
 * to it exactly as it applied to the CLAP quotes that had to be withdrawn.
 * If 65,673 does not survive TIDE_FEEDBACK_TRACE_EVERY=1 then no format ever
 * carried the blob, and E80 is a rack-feedback question every format shares
 * rather than a CLAP defect.
 *
 * WHY A BARE VST3 HOST, AND NOT REAPER. Same reason tests/e80_clap_feedback_
 * probe.c exists: this box's scheduled runs fire whether or not the developer
 * is working at it, and REAPER wants a screen, a `%APPDATA%\REAPER` backup and
 * an idle machine. 2026-09-09 and 2026-09-10 both found him mid-edit and both
 * declined to take the GUI. This runs in ~20 s with no DAW and no window --
 * and, with --editor, no VISIBLE window either. It is the VST3 twin of the
 * CLAP probe and is deliberately the same shape, arm for arm, so that the two
 * can be quoted side by side.
 *
 * THE TWO COUNTERS ARE UPSTREAM OF THE WRAPPER, which is what makes the
 * comparison fair across formats:
 *
 *   - `RackProcessor: '<slug>' display-state capture #N (B bytes)` is raised in
 *     RackAdaptor.h's sendDisplayState(), from the rack module's subProcess().
 *   - `TIDE: instance #N feedback send #M (B bytes, H held back)` is raised in
 *     SynthEditSem/SynthEdit.cpp's drainRackFeedback(), at the end of TIDE's
 *     own subProcess().
 *
 * Neither is gated on an editor, and neither knows which wrapper it is under.
 *
 *   --pump      (default) run the host's main-thread message loop between
 *               blocks. This is what feeds gmpi's SetTimer-backed TimerClient.
 *   --no-pump   never run it. CONTROL: it starves that timer.
 *   --no-preset NEGATIVE CONTROL: never call setState, so the plug-in runs its
 *               bundled default rack, which has no VCV Scope in it. If
 *               `display-state capture` still appears, the lines in the other
 *               arms did not come from the document under test.
 *   --editor    Create the plug-in's editor -- the CONTROLLER, connected to the
 *               component, plus an IPlugView attached to an INVISIBLE,
 *               OFF-SCREEN parent window. Same WS_POPUP trick as the CLAP
 *               probe's --editor arm: never given WS_VISIBLE, no owner, placed
 *               at (-32000,-32000), so nothing appears on the developer's
 *               desktop and nothing can take focus.
 *
 * A NOTE ON WHAT --editor NEEDS THAT --no-editor DOES NOT, because it is not
 * cosmetic on VST3. The wrapper moves DSP->UI traffic by allocating an
 * IMessage from the host (Processor_VST3.cpp's background thread calls
 * allocateMessage(), and `if (!message) break;`). allocateMessage() resolves
 * through IHostApplication on the host context -- so a host that does not
 * offer one has NO DSP->UI channel at all, by construction. This probe
 * therefore implements a minimal IHostApplication (IMessage + IAttributeList,
 * the setInt/getInt and setBinary/getBinary the wrapper actually uses) and
 * offers it in BOTH arms, so the host context is not a variable between them.
 * That is a deliberate difference from the CLAP probe, where the clap.gui host
 * extension IS wired behind the flag -- there it had to be, to keep the
 * control arm byte-identical to the host an earlier run's figures came from;
 * here there is no earlier bare-host figure to stay comparable with.
 *
 * Build (Windows, from a VS x64 developer prompt or with cl on PATH):
 *   cl /std:c++17 /EHsc /nologo /O2 /I C:\SE\SDKs\VST3_SDK ^
 *      tests\e80_vst3_feedback_probe.cpp /Fe:e80vst3probe.exe ^
 *      /link user32.lib ole32.lib
 *
 * Nothing from the VST3 SDK is COMPILED or LINKED -- only its headers are
 * read. The interface IIDs come from the `IFoo_iid` constants DECLARE_CLASS_IID
 * puts at namespace scope; `IFoo::iid` (the FUID member) is NOT used, because
 * that one lives in coreiids.cpp and would drag the SDK's build in.
 *
 * Run, and note that THE EVIDENCE IS ON STDERR, written by the plug-in:
 *   set TIDE_FEEDBACK_TRACE_EVERY=1
 *   e80vst3probe.exe <path-to.vst3> <preset.xml> --blocks 800 2> trace.err
 *   grep -oE "feedback send #[0-9]+ \([0-9]+ bytes" trace.err | sort -u
 *
 * The preset is the OUTER <Preset> element -- the same file the CLAP probe
 * takes, and what tests/fixtures/*.xml are. The VST3 wrapper's setState wants
 * it length-prefixed with an int32 (Processor_VST3.cpp's setState reads a
 * chunkSize and then that many bytes); the CLAP wrapper reads the stream raw.
 * That framing difference is handled here, so the SAME fixture file drives
 * both probes.
 */

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>
#endif

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstmessage.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

static int failures = 0;

static void check(const char* what, bool ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++failures;
}

/* Every object below is owned by main()'s stack frame and outlives the plug-in
 * instance, so reference counting is deliberately a no-op. A wrapper that
 * over-releases a host object would crash a real host and is silently survived
 * here; that is a limitation of the instrument, not a claim about the plug-in. */
#define TIDE_PROBE_REFCOUNT                                     \
    uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return 1; }      \
    uint32 PLUGIN_API release() SMTG_OVERRIDE { return 1; }

static bool iidIs(const TUID a, const TUID b)
{
    return FUnknownPrivate::iidEqual(a, b);
}

/* ---- IBStream over memory, for setState / setComponentState -------------- */

class MemStream : public IBStream
{
public:
    std::string buf;
    int64 pos = 0;

    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IBStream_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IBStream*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    TIDE_PROBE_REFCOUNT

    tresult PLUGIN_API read(void* buffer, int32 numBytes, int32* numBytesRead) SMTG_OVERRIDE
    {
        const int64 left = (int64)buf.size() - pos;
        int64 n = numBytes < left ? (int64)numBytes : left;
        if (n < 0) n = 0;
        memcpy(buffer, buf.data() + pos, (size_t)n);
        pos += n;
        if (numBytesRead) *numBytesRead = (int32)n;
        return kResultOk;
    }

    tresult PLUGIN_API write(void* buffer, int32 numBytes, int32* numBytesWritten) SMTG_OVERRIDE
    {
        if (pos != (int64)buf.size()) buf.resize((size_t)pos);
        buf.append((const char*)buffer, (size_t)numBytes);
        pos = (int64)buf.size();
        if (numBytesWritten) *numBytesWritten = numBytes;
        return kResultOk;
    }

    tresult PLUGIN_API seek(int64 p, int32 mode, int64* result) SMTG_OVERRIDE
    {
        if (mode == kIBSeekSet)      pos = p;
        else if (mode == kIBSeekCur) pos += p;
        else                         pos = (int64)buf.size() + p;
        if (pos < 0) pos = 0;
        if (pos > (int64)buf.size()) pos = (int64)buf.size();
        if (result) *result = pos;
        return kResultOk;
    }

    tresult PLUGIN_API tell(int64* p) SMTG_OVERRIDE
    {
        if (p) *p = pos;
        return kResultOk;
    }
};

/* ---- the minimal IHostApplication the wrapper's DSP->UI channel needs ----
 *
 * Only two attribute types are ever used by this wrapper -- `setInt`/`getInt`
 * for the E68 controller-pointer handshake ("GmpiCtlPtr") and
 * `setBinary`/`getBinary` for the actual payload ("MyData", "Preset"). The
 * rest return kNotImplemented rather than pretending, so a wrapper that starts
 * relying on one fails loudly here instead of silently losing data. */

class AttrList : public IAttributeList
{
    std::map<std::string, int64>       ints;
    std::map<std::string, std::string> bins;

public:
    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IAttributeList_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IAttributeList*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    TIDE_PROBE_REFCOUNT

    tresult PLUGIN_API setInt(AttrID id, int64 value) SMTG_OVERRIDE
    {
        ints[id] = value;
        return kResultOk;
    }
    tresult PLUGIN_API getInt(AttrID id, int64& value) SMTG_OVERRIDE
    {
        auto it = ints.find(id);
        if (it == ints.end()) return kResultFalse;
        value = it->second;
        return kResultOk;
    }
    tresult PLUGIN_API setFloat(AttrID, double) SMTG_OVERRIDE { return kNotImplemented; }
    tresult PLUGIN_API getFloat(AttrID, double&) SMTG_OVERRIDE { return kNotImplemented; }
    tresult PLUGIN_API setString(AttrID, const TChar*) SMTG_OVERRIDE { return kNotImplemented; }
    tresult PLUGIN_API getString(AttrID, TChar*, uint32) SMTG_OVERRIDE { return kNotImplemented; }

    tresult PLUGIN_API setBinary(AttrID id, const void* data, uint32 sizeInBytes) SMTG_OVERRIDE
    {
        bins[id].assign((const char*)data, sizeInBytes);
        return kResultOk;
    }
    tresult PLUGIN_API getBinary(AttrID id, const void*& data, uint32& sizeInBytes) SMTG_OVERRIDE
    {
        auto it = bins.find(id);
        if (it == bins.end()) return kResultFalse;
        data = it->second.data();
        sizeInBytes = (uint32)it->second.size();
        return kResultOk;
    }
};

class HostMessage : public IMessage
{
    std::string id;
    AttrList    attrs;

public:
    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IMessage_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IMessage*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }

    /* Messages are the ONE host object here that is genuinely owned by the
     * plug-in: allocateMessage() hands it over and the wrapper FReleaser's it.
     * So these are real, and a leak means the wrapper never released one. */
    uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return ++rc; }
    uint32 PLUGIN_API release() SMTG_OVERRIDE
    {
        if (--rc == 0) { delete this; return 0; }
        return rc;
    }
    int32 rc = 1;

    FIDString PLUGIN_API getMessageID() SMTG_OVERRIDE { return id.c_str(); }
    void PLUGIN_API setMessageID(FIDString mid) SMTG_OVERRIDE { id = mid ? mid : ""; }
    IAttributeList* PLUGIN_API getAttributes() SMTG_OVERRIDE { return &attrs; }
};

static int messagesAllocated = 0;

class HostApp : public IHostApplication
{
public:
    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IHostApplication_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IHostApplication*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    TIDE_PROBE_REFCOUNT

    tresult PLUGIN_API getName(String128 name) SMTG_OVERRIDE
    {
        static const char16_t n[] = u"tide-e80-vst3-probe";
        memcpy(name, n, sizeof(n));
        return kResultOk;
    }

    tresult PLUGIN_API createInstance(TUID cid, TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(cid, IMessage_iid) && iidIs(_iid, IMessage_iid))
        {
            ++messagesAllocated;
            *obj = static_cast<IMessage*>(new HostMessage());
            return kResultOk;
        }
        if (iidIs(cid, IAttributeList_iid) && iidIs(_iid, IAttributeList_iid))
        {
            *obj = static_cast<IAttributeList*>(new AttrList());
            return kResultOk;
        }
        *obj = nullptr;
        return kResultFalse;
    }
};

/* ---- events: a single note-on, so the rack's envelopes actually open ----- */

class EventList : public IEventList
{
public:
    Event ev{};
    bool  deliver = false;

    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IEventList_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IEventList*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    TIDE_PROBE_REFCOUNT

    int32 PLUGIN_API getEventCount() SMTG_OVERRIDE { return deliver ? 1 : 0; }
    tresult PLUGIN_API getEvent(int32 index, Event& e) SMTG_OVERRIDE
    {
        if (!deliver || index != 0) return kResultFalse;
        e = ev;
        return kResultOk;
    }
    tresult PLUGIN_API addEvent(Event&) SMTG_OVERRIDE { return kNotImplemented; }
};

/* ---- the editor arm's host side ----------------------------------------- */

class ComponentHandler : public IComponentHandler
{
public:
    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IComponentHandler_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IComponentHandler*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    TIDE_PROBE_REFCOUNT

    tresult PLUGIN_API beginEdit(ParamID) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API performEdit(ParamID, ParamValue) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API endEdit(ParamID) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API restartComponent(int32) SMTG_OVERRIDE { return kResultOk; }
};

class PlugFrame : public IPlugFrame
{
public:
    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) SMTG_OVERRIDE
    {
        if (iidIs(_iid, IPlugFrame_iid) || iidIs(_iid, FUnknown_iid))
        {
            *obj = static_cast<IPlugFrame*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    TIDE_PROBE_REFCOUNT

    /* Accepted and ignored: the parent is invisible, so its size is not
     * observable and resizing it would only hide a mismatch. */
    tresult PLUGIN_API resizeView(IPlugView*, ViewRect*) SMTG_OVERRIDE { return kResultOk; }
};

/* ---- the invisible parent ------------------------------------------------
 *
 * Identical in intent to tests/e80_clap_feedback_probe.c's, and for the same
 * reason: this box's scheduled runs fire whether or not the developer is
 * working at it, so an editor arm that stole focus would be unrunnable on
 * exactly the days it is scheduled. WS_POPUP, never given WS_VISIBLE, no
 * owner, no WS_EX_APPWINDOW, off-screen origin. The plug-in's window is a
 * CHILD of it, and a child of a window that was never shown is not shown
 * either, whatever the child does with ShowWindow.
 *
 * What that costs, and it must be stated wherever a figure from this arm is
 * quoted: an invisible window gets no WM_PAINT, so nothing that depends on the
 * editor having actually PAINTED is observable. E80's far-end counter is not
 * one of those -- RackEditor.h raises `display-state update #N arrived` from
 * the pin-set path, not from render(). */
#if defined(_WIN32)
static const char* kProbeWndClass = "TideE80Vst3ProbeParent";

static LRESULT CALLBACK probe_wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    return DefWindowProcA(h, m, w, l);
}

static HWND probe_make_hidden_parent(int w, int h)
{
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc   = probe_wndproc;
    wc.hInstance     = GetModuleHandleA(nullptr);
    wc.lpszClassName = kProbeWndClass;
    RegisterClassA(&wc);          /* a repeat registration fails harmlessly */

    return CreateWindowExA(
        0, kProbeWndClass, "tide e80 vst3 probe (never shown)",
        WS_POPUP | WS_CLIPCHILDREN,
        -32000, -32000, w, h,
        nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
}
#endif

/* Let the host's main thread run, the way a real host's does between audio
 * callbacks. gmpi's TimerClient is SetTimer-backed on Windows, and WM_TIMER is
 * a synthesised, lowest-priority message that PeekMessage only reports when the
 * queue is otherwise empty -- precisely the condition a bare host is always in.
 * Loop for the requested slice rather than draining once, so a 15 ms tick
 * inside a 1 ms slice is not missed by construction. */
static void pump_main_thread(double seconds)
{
#if defined(_WIN32)
    const DWORD until = GetTickCount() + (DWORD)(seconds * 1000.0);
    for (;;)
    {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (GetTickCount() >= until) break;
        Sleep(1);
    }
#else
    (void)seconds;
#endif
}

static bool slurp(const char* path, std::string& out)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize((size_t)(n > 0 ? n : 0));
    size_t rd = out.empty() ? 0 : fread(&out[0], 1, out.size(), f);
    out.resize(rd);
    fclose(f);
    return true;
}

int main(int argc, char** argv)
{
    const char* bundle = nullptr;
    const char* presetPath = nullptr;
    bool usePump = true, loadPreset = true, wantEditor = false;
    int  blocks = 800;
    const int32  blockSize = 512;
    const double sampleRate = 44100.0;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--pump"))            usePump = true;
        else if (!strcmp(argv[i], "--no-pump"))    usePump = false;
        else if (!strcmp(argv[i], "--no-preset"))  loadPreset = false;
        else if (!strcmp(argv[i], "--editor"))     wantEditor = true;
        else if (!strcmp(argv[i], "--no-editor"))  wantEditor = false;
        else if (!strcmp(argv[i], "--blocks") && i + 1 < argc) blocks = atoi(argv[++i]);
        else if (!bundle)     bundle = argv[i];
        else if (!presetPath) presetPath = argv[i];
    }
    if (!bundle || (!presetPath && loadPreset))
    {
        fprintf(stderr,
                "usage: %s <path-to.vst3> <preset.xml> [--pump|--no-pump] [--no-preset]\n"
                "       [--editor|--no-editor] [--blocks N]\n",
                argv[0]);
        return 2;
    }

    printf("e80_vst3_feedback_probe: arm = %s%s, %d blocks of %d at %.0f Hz, %s\n\n",
           usePump ? "--pump (a host main thread runs)"
                   : "--no-pump (starves the controller's timer)",
           loadPreset ? "" : " + --no-preset (NEGATIVE CONTROL: the bundled default rack)",
           blocks, blockSize, sampleRate,
           wantEditor ? "EDITOR CREATED (embedded in an invisible off-screen parent)"
                      : "editor NEVER created");

#if !defined(_WIN32)
    fprintf(stderr, "this probe is win32-only; mac/linux load a .vst3 BUNDLE, "
                    "not a bare shared library\n");
    return 2;
#else

    /* A .vst3 on Windows may be a bundle DIRECTORY or -- as TIDE builds it --
     * a plain DLL whose extension happens to be .vst3, which LoadLibrary is
     * perfectly happy with. */
    HMODULE lib = LoadLibraryA(bundle);
    check("the plug-in binary loads", lib != nullptr);
    if (!lib)
    {
        fprintf(stderr, "  LoadLibrary(%s) failed, GetLastError=%lu\n",
                bundle, (unsigned long)GetLastError());
        return 1;
    }

    /* InitDll is optional and TIDE's .def exports only GetPluginFactory, so a
     * missing InitDll is expected rather than a failure. Called when present
     * because a host does. */
    typedef bool (PLUGIN_API * InitDllProc)();
    typedef IPluginFactory* (PLUGIN_API * GetFactoryProc)();

    if (auto initDll = (InitDllProc)GetProcAddress(lib, "InitDll"))
        printf("      InitDll() -> %s\n", initDll() ? "true" : "false");
    else
        printf("      no InitDll export (expected: TIDE exports GetPluginFactory only)\n");

    auto getFactory = (GetFactoryProc)GetProcAddress(lib, "GetPluginFactory");
    check("GetPluginFactory is exported", getFactory != nullptr);
    if (!getFactory) return 1;

    IPluginFactory* factory = getFactory();
    check("GetPluginFactory returns a factory", factory != nullptr);
    if (!factory) return 1;

    /* Find the audio-effect class. A VST3 factory also advertises the
     * controller as its own class, so picking index 0 blindly is a coin toss. */
    TUID componentCid;
    memset(componentCid, 0, sizeof componentCid);
    bool haveCid = false;
    const int32 classCount = factory->countClasses();
    printf("      the factory advertises %d class(es)\n", (int)classCount);
    for (int32 i = 0; i < classCount && !haveCid; ++i)
    {
        PClassInfo ci;
        if (factory->getClassInfo(i, &ci) != kResultOk) continue;
        printf("        [%d] %-24s category=%s\n", (int)i, ci.name, ci.category);
        if (!strcmp(ci.category, kVstAudioEffectClass))
        {
            memcpy(componentCid, ci.cid, sizeof(TUID));
            haveCid = true;
        }
    }
    check("an Audio Module Class is advertised", haveCid);
    if (!haveCid) return 1;

    HostApp hostApp;

    IComponent* component = nullptr;
    check("createInstance(IComponent) succeeds",
          factory->createInstance(componentCid, IComponent_iid, (void**)&component) == kResultOk
              && component != nullptr);
    if (!component) return 1;

    check("component->initialize succeeds",
          component->initialize(static_cast<IHostApplication*>(&hostApp)) == kResultOk);

    IAudioProcessor* processor = nullptr;
    check("the component offers IAudioProcessor",
          component->queryInterface(IAudioProcessor_iid, (void**)&processor) == kResultOk
              && processor != nullptr);
    if (!processor) return 1;

    /* ---- the state, in the framing the VST3 wrapper wants ----------------
     * Processor_VST3::setState reads an int32 chunkSize and then that many
     * bytes. The CLAP wrapper reads the whole stream raw. Same fixture file,
     * different envelope -- handled here so both probes take the same input. */
    MemStream stateStream;
    if (loadPreset)
    {
        std::string preset;
        check("the preset file reads", slurp(presetPath, preset) && !preset.empty());
        if (preset.empty()) return 1;
        printf("      loading a %zu byte preset (int32-length-prefixed for VST3)\n",
               preset.size());

        const int32 chunkSize = (int32)preset.size();
        stateStream.buf.assign((const char*)&chunkSize, sizeof(chunkSize));
        stateStream.buf.append(preset);
        stateStream.pos = 0;

        check("component->setState accepts the preset",
              component->setState(&stateStream) == kResultTrue);
    }
    else
    {
        printf("      NO setState at all (negative control: the DEFAULT rack)\n");
    }

    /* THE HANDOVER WINDOW. A real host restores state and then goes back to
     * its event loop before the first audio callback. The control arm skips it. */
    if (usePump)
    {
        printf("      pumping the main thread for 0.5 s (the host's restore->play gap)\n");
        pump_main_thread(0.5);
    }
    else
    {
        printf("      NOT pumping the main thread (control arm)\n");
    }

    /* ---- --editor: controller, connection, view -- BEFORE setActive -------
     *
     * Order matters and this is the host-like one: a DAW restores state, the
     * user has the editor open, and then the transport rolls. Creating it
     * after setActive would also work, but it would make "the editor missed
     * the first N blocks" a live explanation for any shortfall, and this arm
     * exists to REMOVE explanations, not add them. */
    IEditController* controller = nullptr;
    IPlugView*       view = nullptr;
    IConnectionPoint* cpComponent = nullptr;
    IConnectionPoint* cpController = nullptr;
    ComponentHandler  handler;
    PlugFrame         frame;
    HWND parentWnd = nullptr;
    bool viewAttached = false;

    if (wantEditor)
    {
        TUID controllerCid;
        memset(controllerCid, 0, sizeof controllerCid);
        const bool haveCtlCid = component->getControllerClassId(controllerCid) == kResultOk;
        check("the component names a controller class", haveCtlCid);

        if (haveCtlCid)
        {
            check("createInstance(IEditController) succeeds",
                  factory->createInstance(controllerCid, IEditController_iid,
                                          (void**)&controller) == kResultOk
                      && controller != nullptr);
        }

        if (controller)
        {
            check("controller->initialize succeeds",
                  controller->initialize(static_cast<IHostApplication*>(&hostApp)) == kResultOk);
            check("controller->setComponentHandler succeeds",
                  controller->setComponentHandler(&handler) == kResultOk);

            /* THE CONNECTION IS NOT OPTIONAL ON THIS WRAPPER. DSP->UI traffic
             * is IMessage-based: Processor_VST3 allocates a message from the
             * host and sends it over this connection. No connection, no far
             * end -- and the probe would be measuring its own omission. */
            if (component->queryInterface(IConnectionPoint_iid, (void**)&cpComponent) != kResultOk)
                cpComponent = nullptr;
            if (controller->queryInterface(IConnectionPoint_iid, (void**)&cpController) != kResultOk)
                cpController = nullptr;

            check("both ends offer IConnectionPoint",
                  cpComponent != nullptr && cpController != nullptr);
            if (cpComponent && cpController)
            {
                check("component->connect(controller) succeeds",
                      cpComponent->connect(cpController) == kResultOk);
                check("controller->connect(component) succeeds",
                      cpController->connect(cpComponent) == kResultOk);
            }

            if (loadPreset)
            {
                stateStream.pos = 0;
                const auto r = controller->setComponentState(&stateStream);
                printf("      controller->setComponentState -> %s\n",
                       r == kResultTrue ? "kResultTrue" : "not kResultTrue");
            }

            view = controller->createView(ViewType::kEditor);
            check("controller->createView(editor) returns a view", view != nullptr);
        }

        if (view)
        {
            check("the view supports the HWND platform type",
                  view->isPlatformTypeSupported(kPlatformTypeHWND) == kResultTrue);

            ViewRect rect{};
            if (view->getSize(&rect) == kResultOk && rect.getWidth() > 0 && rect.getHeight() > 0)
                printf("      view->getSize -> %dx%d\n", rect.getWidth(), rect.getHeight());
            else
            {
                rect.left = rect.top = 0;
                rect.right = 1024;
                rect.bottom = 768;
                printf("      view->getSize declined; using %dx%d for the parent\n",
                       rect.getWidth(), rect.getHeight());
            }

            view->setFrame(&frame);

            parentWnd = probe_make_hidden_parent(rect.getWidth(), rect.getHeight());
            check("an invisible off-screen parent window was created", parentWnd != nullptr);
            printf("      parent HWND is %s (IsWindowVisible=%d) -- nothing appears on screen\n",
                   parentWnd ? "valid" : "NULL",
                   parentWnd ? (int)IsWindowVisible(parentWnd) : -1);

            if (parentWnd)
            {
                viewAttached = view->attached((void*)parentWnd, kPlatformTypeHWND) == kResultOk;
                check("view->attached succeeds", viewAttached);

                if (viewAttached)
                {
                    /* Give the editor a slice to build itself and bind to the
                     * controller before the first process() call. */
                    pump_main_thread(0.5);
                    int n = 0;
                    for (HWND c = GetWindow(parentWnd, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT))
                        ++n;
                    printf("      the plug-in created %d child window(s) under it "
                           "(parent IsWindowVisible=%d)\n",
                           n, (int)IsWindowVisible(parentWnd));
                }
            }
        }
    }

    /* ---- buses, setup, activate ----------------------------------------- */

    const int32 audioOutBuses = component->getBusCount(kAudio, kOutput);
    const int32 audioInBuses  = component->getBusCount(kAudio, kInput);
    const int32 eventInBuses  = component->getBusCount(kEvent, kInput);
    printf("      buses: audio in %d, audio out %d, event in %d\n",
           (int)audioInBuses, (int)audioOutBuses, (int)eventInBuses);

    for (int32 i = 0; i < audioOutBuses; ++i) component->activateBus(kAudio, kOutput, i, true);
    for (int32 i = 0; i < eventInBuses;  ++i) component->activateBus(kEvent, kInput,  i, true);

    ProcessSetup setup{};
    setup.processMode        = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = blockSize;
    setup.sampleRate         = sampleRate;
    check("setupProcessing succeeds", processor->setupProcessing(setup) == kResultOk);

    check("component->setActive(true) succeeds", component->setActive(true) == kResultOk);
    check("setProcessing(true) succeeds", processor->setProcessing(true) == kResultOk);

    /* Two channels per bus is what the wrapper's outputsAsStereoPairs builds,
     * and getBusInfo would only confirm it; allocate for the buses that exist. */
    const int32 outChannels = 2;
    std::vector<std::vector<float>> outStore((size_t)(audioOutBuses * outChannels),
                                             std::vector<float>((size_t)blockSize, 0.0f));
    std::vector<std::vector<float*>> outPtrs((size_t)audioOutBuses,
                                             std::vector<float*>((size_t)outChannels, nullptr));
    std::vector<AudioBusBuffers> outBuses((size_t)(audioOutBuses > 0 ? audioOutBuses : 1));
    for (int32 b = 0; b < audioOutBuses; ++b)
    {
        for (int32 c = 0; c < outChannels; ++c)
            outPtrs[(size_t)b][(size_t)c] = outStore[(size_t)(b * outChannels + c)].data();
        outBuses[(size_t)b].numChannels       = outChannels;
        outBuses[(size_t)b].silenceFlags      = 0;
        outBuses[(size_t)b].channelBuffers32  = outPtrs[(size_t)b].data();
    }

    EventList events;
    events.ev.busIndex       = 0;
    events.ev.sampleOffset   = 0;
    events.ev.ppqPosition    = 0.0;
    events.ev.flags          = 0;
    events.ev.type           = Event::kNoteOnEvent;
    events.ev.noteOn.channel  = 0;
    events.ev.noteOn.pitch    = 60;      /* middle C, the note V1's pitch clause used */
    events.ev.noteOn.tuning   = 0.0f;
    events.ev.noteOn.velocity = 0.8f;
    events.ev.noteOn.length   = 0;
    events.ev.noteOn.noteId   = -1;

    double peak = 0.0, sumsq = 0.0;
    uint64 frames = 0;
    int    processFailures = 0;

    for (int b = 0; b < blocks; ++b)
    {
        events.deliver = (b == 2);

        for (auto& ch : outStore) memset(ch.data(), 0, ch.size() * sizeof(float));

        ProcessData data{};
        data.processMode            = kRealtime;
        data.symbolicSampleSize     = kSample32;
        data.numSamples             = blockSize;
        data.numInputs              = 0;
        data.numOutputs             = audioOutBuses;
        data.inputs                 = nullptr;
        data.outputs                = audioOutBuses > 0 ? outBuses.data() : nullptr;
        data.inputParameterChanges  = nullptr;
        data.outputParameterChanges = nullptr;
        data.inputEvents            = &events;
        data.outputEvents           = nullptr;
        data.processContext         = nullptr;

        if (processor->process(data) != kResultOk) ++processFailures;

        if (audioOutBuses > 0)
        {
            for (int32 c = 0; c < outChannels; ++c)
            {
                const float* p = outPtrs[0][(size_t)c];
                for (int32 i = 0; i < blockSize; ++i)
                {
                    const double a = fabs((double)p[i]);
                    if (a > peak) peak = a;
                    sumsq += (double)p[i] * p[i];
                }
            }
        }
        frames += (uint64)blockSize;

        if (usePump) pump_main_thread(0.001);
    }

    check("no block returned an error", processFailures == 0);

    /* The audio figures are NOT this probe's subject -- they are here because
     * they cost nothing and because silence would tell you the document never
     * reached the DSP, which would make every feedback number meaningless.
     * Note that e75-vcv-visible-rack.xml has NO path to an audio output, so
     * -inf dBFS is correct there and alarming anywhere else. */
    const double rms = sqrt(sumsq / (double)(frames * (uint64)outChannels));
    printf("\n      %.3f s rendered: peak %.6f (%.1f dBFS), rms %.1f dBFS\n",
           (double)frames / sampleRate, peak,
           peak > 0.0 ? 20.0 * log10(peak) : -1000.0,
           rms  > 0.0 ? 20.0 * log10(rms)  : -1000.0);

    printf("      host allocated %d IMessage(s) for the wrapper's DSP->UI channel\n",
           messagesAllocated);

    processor->setProcessing(false);
    component->setActive(false);

    if (view)
    {
        if (viewAttached) view->removed();
        view->release();
    }
    if (parentWnd) DestroyWindow(parentWnd);

    if (cpComponent && cpController)
    {
        cpComponent->disconnect(cpController);
        cpController->disconnect(cpComponent);
    }
    if (cpComponent)  cpComponent->release();
    if (cpController) cpController->release();

    if (controller)
    {
        controller->terminate();
        controller->release();
    }

    processor->release();
    component->terminate();
    component->release();

    printf("\n%s -- the numbers that matter are on STDERR, from the plug-in:\n"
           "  RackProcessor: '<slug>' display-state capture #N (B bytes)   <- the DSP captured it\n"
           "  TIDE: instance #N feedback send #M (B bytes, H held back)    <- what the queue carried\n"
           "%s"
           "\nSet TIDE_FEEDBACK_TRACE_EVERY=1 or the second line is SAMPLED (#0,#1,#2 and\n"
           "every 100th), which cannot answer whether the blob ever crossed.\n",
           failures ? "SOME CHECKS FAILED" : "all probe checks passed",
           wantEditor
             ? "  RackEditor: display-state update #N arrived (B bytes)       <- E80's Accept, the FAR end\n"
               "  RackEditor: light N update #M value V                       <- the small-payload control\n"
             : "");

    return failures ? 1 : 0;
#endif
}
