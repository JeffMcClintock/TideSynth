/* BACKLOG E91 -- does a state restore that arrives AFTER allocateRenderResources
 * reach TIDE Rack's DSP on the AUv3 wrapper?
 *
 * WHY THIS EXISTS
 * ---------------
 * E88 proved that on VST3 a restore arriving after activation never reached the
 * DSP: setState wrote the parameter STORES and nothing announced the change --
 * GMPI's own words, "blobs only reach it when they CHANGE". The fix
 * (GMPI_Wrappers#42) made the restore look to the DSP like a live parameter
 * change. E91 asks the same question of AU2 and AU3.
 *
 * AU3's restore is shaped differently from VST3's, and that is the hypothesis
 * this probe tests rather than assumes. GmpiAudioUnit -setFullState: does not
 * call setPresetUnsafe on the processor at all. It writes the CONTROLLER's
 * store and then frames every stateful parameter onto the ui->dsp queue
 * (sendParameterToProcessorQueue, "ppc3" for a blob), which the render block
 * drains on its first line. That is already the live-change route, so by
 * reading the order should not matter. E88's own history is the warning
 * against stopping at reading: a prediction drawn from CLAP's structure did not
 * transfer to VST3 on the same build.
 *
 * (AU2 is not measured here. TIDE does not build AU2 -- S40 dropped it. Read,
 * not measured: AU2's RestoreState hands the GMPIPRESET only to the
 * controller's store, and its ui->dsp queue sender is commented out, so a blob
 * parameter would not reach the DSP in EITHER order. Filed separately.)
 *
 * WHY IT HOSTS THE AU IN-PROCESS, AND HOW
 * ---------------------------------------
 * Loading the registered extension through AudioComponent is out-of-process on
 * macOS, so the plug-in's stderr -- the `TIDE: ... building rack` line that is
 * this question's best evidence -- never reaches the host. And the registered
 * extension on a developer's machine is whatever build they last installed,
 * which a scheduled run must not displace (2026-08-29 macos entry).
 *
 * So this file is linked INTO the appex's own objects instead: the same link
 * line as the TIDE_Rack_AU3 target, with this file's main() replacing
 * `-e _NSExtensionMain`, and the executable placed inside a COPY of the built
 * TIDE-Rack.appex (Contents/MacOS/) so NSBundle resolves the plug-in's
 * resources exactly as it does in the extension. It then instantiates the
 * wrapper's AUAudioUnit subclass by name. Nothing is registered and nothing
 * outside the build tree is touched. See "BUILD" for the recipe.
 *
 * ARMS
 * ----
 *   --order state-first     setFullState, then allocateRenderResources, render
 *   --order activate-first  allocateRenderResources, render a pre-window,
 *                           THEN setFullState, render
 *   --no-preset             allocate and render with no restore at all
 *                           (negative control: must be silent)
 *   --no-readback           do not read fullState back after setting it. The
 *                           readback is NOT passive -- -fullState runs
 *                           syncState(), which re-delivers TIDE's document --
 *                           so only this form isolates setFullState itself.
 *
 * Every arm prints the peak over the measured window. The plug-in's own stderr
 * lines are interleaved in the same stream, so `building rack` either appears
 * after the restore or does not.
 *
 * BUILD (macOS, in a TideSynth `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release`
 * build dir that has built SynthEditSem/TIDE-Rack.appex; $TS is the checkout)
 *
 *   ninja -t commands SynthEditSem/TIDE-Rack.appex/Contents/MacOS/TIDE-Rack \
 *     | tail -1 > link.sh
 *   clang++ -std=c++17 -O2 -fobjc-arc -arch arm64 -mmacosx-version-min=13.3 \
 *     -c $TS/tests/e91_au3_restore_order_probe.mm -o e91probe.o
 *   ditto SynthEditSem/TIDE-Rack.appex host.appex
 *   sed -e 's/ -e _NSExtensionMain//' \
 *       -e 's# -o SynthEditSem/TIDE-Rack.appex/Contents/MacOS/TIDE-Rack# e91probe.o -o host.appex/Contents/MacOS/e91probe#' \
 *       link.sh | bash
 *
 * The link line is the appex's own, so the probe carries exactly the objects
 * the extension does. Dropping `-e _NSExtensionMain` makes main() below the
 * entry point; nothing else changes.
 *
 * RUN
 *   python3 $TS/scripts/decode_rpp.py --preset-out p.xml $TS/tests/hosts/v1-rack.rpp
 *   host.appex/Contents/MacOS/e91probe --preset p.xml --order state-first    --no-readback
 *   host.appex/Contents/MacOS/e91probe --preset p.xml --order activate-first --no-readback
 *   host.appex/Contents/MacOS/e91probe --no-preset
 *
 * MEASURED 2026-10-07 (macos), GMPI_Wrappers 0a791ad, v1-rack's 18,893-byte
 * preset, 400 blocks of 512 at 44.1 kHz, 3 runs per arm, interleaved:
 *
 *   arm                               building rack   peak
 *   state-first    --no-readback      x1              0.482431 (-6.3 dBFS)
 *   activate-first --no-readback      x1, AFTER the   0.482431 (-6.3 dBFS)
 *                                     restore line
 *   --no-preset (negative control)    none            0 (-inf)
 *
 *   CONTROL, setFullState's sendParameterToProcessorQueue line deleted:
 *   state-first / activate-first --no-readback: no build, -inf, BOTH orders.
 *   So that one line is the route, and this probe sees its absence.
 *
 * 0.482431 is the same six digits VST3 and CLAP measure for this document.
 */
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;

static void check(const char* what, bool ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    fflush(stdout);
    if (!ok) ++failures;
}

static double dbfs(double linear)
{
    return linear > 0.0 ? 20.0 * std::log10(linear) : -INFINITY;
}

// Pump the main run loop on the clock. CFRunLoopRunInMode returns at once on a
// loop with no sources yet (2026-10-05 macos lesson), so a single call would
// wait for nothing; the AU3 core's timer may not exist until the unit is up.
static void pump(double seconds)
{
    NSDate* until = [NSDate dateWithTimeIntervalSinceNow:seconds];
    while ([until timeIntervalSinceNow] > 0)
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
}

// Render `blocks` blocks of `blockFrames` and return the peak of channel 0.
// Reads back mData rather than our own buffer: the wrapper may substitute its
// own storage (AU3Core::outputStorage exists for exactly that).
static double renderPeak(AUAudioUnit* au, int channels, AUAudioFrameCount blockFrames,
                         int blocks, AudioTimeStamp& ts, OSStatus& status)
{
    status = noErr;
    const size_t ablBytes = offsetof(AudioBufferList, mBuffers) + sizeof(AudioBuffer) * channels;
    std::vector<uint8_t> ablStore(ablBytes, 0);
    AudioBufferList* abl = (AudioBufferList*)ablStore.data();
    abl->mNumberBuffers = (UInt32)channels;
    std::vector<std::vector<float>> chan(channels, std::vector<float>(blockFrames, 0.0f));

    AURenderBlock render = au.renderBlock;
    if (!render) { status = kAudioUnitErr_Uninitialized; return 0.0; }

    double peak = 0.0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int c = 0; c < channels; ++c)
        {
            std::fill(chan[c].begin(), chan[c].end(), 0.0f);
            abl->mBuffers[c].mNumberChannels = 1;
            abl->mBuffers[c].mDataByteSize   = blockFrames * sizeof(float);
            abl->mBuffers[c].mData           = chan[c].data();
        }
        AudioUnitRenderActionFlags flags = 0;
        const OSStatus st = render(&flags, &ts, blockFrames, 0, abl, nil);
        if (st != noErr) { status = st; return peak; }

        const float* src = (const float*)abl->mBuffers[0].mData;
        if (src)
            for (AUAudioFrameCount i = 0; i < blockFrames; ++i)
                peak = std::max(peak, (double)std::fabs(src[i]));

        ts.mSampleTime += blockFrames;
    }
    return peak;
}

int main(int argc, const char* argv[])
{
    @autoreleasepool
    {
        setvbuf(stdout, nullptr, _IOLBF, 0);

        const char* presetPath = nullptr;
        std::string order = "state-first";
        bool noPreset = false;
        bool readback = true;
        int blocks = 400;
        const char* className = "GmpiAudioUnit";

        for (int i = 1; i < argc; ++i)
        {
            if (!std::strcmp(argv[i], "--preset") && i + 1 < argc) presetPath = argv[++i];
            else if (!std::strcmp(argv[i], "--order") && i + 1 < argc) order = argv[++i];
            else if (!std::strcmp(argv[i], "--no-preset")) noPreset = true;
            else if (!std::strcmp(argv[i], "--no-readback")) readback = false;
            else if (!std::strcmp(argv[i], "--blocks") && i + 1 < argc) blocks = std::atoi(argv[++i]);
            else if (!std::strcmp(argv[i], "--class") && i + 1 < argc) className = argv[++i];
            else { fprintf(stderr, "unknown argument: %s\n", argv[i]); return 2; }
        }

        if (order != "state-first" && order != "activate-first")
        {
            fprintf(stderr, "--order must be state-first or activate-first\n");
            return 2;
        }
        if (!noPreset && !presetPath)
        {
            fprintf(stderr, "usage: %s --preset <preset.xml> --order state-first|activate-first | --no-preset\n", argv[0]);
            return 2;
        }

        NSString* preset = nil;
        if (!noPreset)
        {
            preset = [NSString stringWithContentsOfFile:@(presetPath) encoding:NSUTF8StringEncoding error:nil];
            if (!preset.length) { fprintf(stderr, "cannot read preset %s\n", presetPath); return 2; }
        }

        printf("--- E91 AU3 restore-order probe: arm=%s%s ---\n",
               noPreset ? "no-preset" : order.c_str(), readback ? "" : " --no-readback");
        printf("     main bundle: %s\n", [[NSBundle mainBundle] bundlePath].UTF8String);
        if (preset) printf("     preset: %lu bytes from %s\n", (unsigned long)preset.length, presetPath);

        // The wrapper's AUAudioUnit subclass, linked into THIS executable.
        Class cls = NSClassFromString(@(className));
        check("the wrapper's AUAudioUnit subclass is linked into this process", cls != nil);
        if (!cls) return 2;
        check("it IS an AUAudioUnit subclass", [cls isSubclassOfClass:[AUAudioUnit class]]);

        AudioComponentDescription desc{};
        desc.componentType         = kAudioUnitType_MusicDevice;      // aumu
        desc.componentSubType      = 'Drck';
        desc.componentManufacturer = 'Dsyh';

        NSError* err = nil;
        AUAudioUnit* au = [[cls alloc] initWithComponentDescription:desc options:0 error:&err];
        check("AUAudioUnit instantiated in-process", au != nil);
        if (!au) { fprintf(stderr, "     error: %s\n", err.localizedDescription.UTF8String); return 1; }
        pump(0.5);

        AUAudioUnitBus* outBus = au.outputBusses.count > 0 ? au.outputBusses[0] : nil;
        check("the unit has an output bus", outBus != nil);
        if (!outBus) return 1;
        const int channels = (int)outBus.format.channelCount;
        const double rate = 44100.0;
        AVAudioFormat* fmt = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:rate
                                                                            channels:(AVAudioChannelCount)channels];
        check("output bus accepts 44100 Hz", [outBus setFormat:fmt error:nil]);

        const AUAudioFrameCount blockFrames = 512;
        au.maximumFramesToRender = blockFrames;

        auto restore = [&]()
        {
            printf("     >>> setFullState (%lu-byte GMPIPRESET)\n", (unsigned long)preset.length);
            au.fullState = @{ @"GMPIPRESET" : preset };
            // Reading it back proves the property took (S33's lesson) -- BUT
            // IT IS NOT A PASSIVE READ. -fullState calls syncState() (E68),
            // and TIDE's syncState re-exports the document through the
            // parameter path, which is itself a delivery route to the DSP.
            // Measured 2026-10-07: with setFullState's own queue send deleted,
            // the readback alone still got the rack built. So the readback is
            // an arm of its own, and --no-readback is the honest restore: a
            // host that sets state and does not immediately ask for it back.
            if (readback)
            {
                NSString* rb = au.fullState[@"GMPIPRESET"];
                check("fullState round-trips a non-empty GMPIPRESET",
                      [rb isKindOfClass:[NSString class]] && rb.length > 0);
            }
            else
            {
                printf("     (no readback: fullState is not read after the restore)\n");
            }
            // Give the controller side its main-thread time, as a host would.
            pump(0.5);
        };

        AudioTimeStamp ts{};
        ts.mFlags = kAudioTimeStampSampleTimeValid;
        OSStatus st = noErr;

        if (!noPreset && order == "state-first")
            restore();

        printf("     >>> allocateRenderResources\n");
        NSError* ae = nil;
        const bool allocOk = [au allocateRenderResourcesAndReturnError:&ae];
        check("allocateRenderResources", allocOk);
        if (!allocOk) { fprintf(stderr, "     error: %s\n", ae.localizedDescription.UTF8String); return 1; }

        if (!noPreset && order == "activate-first")
        {
            // The pre-window: the unit is live and rendering with no document.
            // Recorded, not asserted -- what matters is what happens after the
            // restore -- but it shows the restore really did land on a running
            // unit rather than before the first render.
            const double pre = renderPeak(au, channels, blockFrames, 50, ts, st);
            printf("     pre-restore window: 50 blocks, peak %.6f (%.1f dBFS), status %d\n",
                   pre, dbfs(pre), (int)st);
            restore();
        }

        const double peak = renderPeak(au, channels, blockFrames, blocks, ts, st);
        check("render returns noErr", st == noErr);
        printf("     MEASURED: %d blocks of %u at %.0f Hz, peak %.6f (%.1f dBFS)\n",
               blocks, blockFrames, rate, peak, dbfs(peak));

        if (noPreset)
            check("negative control: no document renders digital silence", peak == 0.0);
        else
            check("the restored rack is AUDIBLE", peak > 0.0);

        [au deallocateRenderResources];
        au = nil;
        pump(0.2);

        printf("\n%s: %d check(s) failed\n", failures ? "FAILED" : "OK", failures);
        return failures ? 1 : 0;
    }
}
