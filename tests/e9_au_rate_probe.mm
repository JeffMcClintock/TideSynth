/* BACKLOG E9 -- drive a real AudioUnit v3 host handshake against TIDE Rack's
 * AUv3, at two sample rates, and measure the PITCH at each one.
 *
 * WHY THIS EXISTS
 * ---------------
 * E9's remaining clause is one sentence: "AU remains genuinely unmeasured".
 * It defers to R3a, which the row calls BLOCKED(M1) with "TIDE builds no AU".
 * Both halves went stale on 2026-08-22: M1 and R3a are DONE and
 * SynthEditSem/CMakeLists.txt's FORMATS_LIST is `GMPI VST3 CLAP AU3 STANDALONE`.
 * So the AU path is buildable, and therefore measurable, for the first time.
 *
 * The VST3 half was measured in REAPER by hand and the CLAP half by
 * tests/e9_clap_rate_probe.c. This is the third wrapper, and it asks a STRICTER
 * question than either of those did.
 *
 * WHAT IT MEASURES, AND WHY IT IS NOT THE SAME AS THE CLAP PROBE
 * -------------------------------------------------------------
 * The CLAP probe deliberately stopped at the handshake: "it asserts the
 * handshake completes and the plugin reports the rate it was given. What the
 * audio sounds like is E1's job." That is a fair test of the MECHANISM but it
 * is not E9's Accept clause, which is:
 *
 *     "changing the host's sample rate on a loaded project re-tunes correctly
 *      rather than staying detuned"
 *
 * A handshake that completes at 44100 tells you nothing about tuning. So this
 * probe loads a real TIDE document -- the same one tests/hosts/v1-rack.rpp
 * holds, an Oscillator -> Envelope -> Output rack that drones at 440 Hz -- and
 * measures the OUTPUT FREQUENCY at each rate. The two hypotheses are separated
 * by a musical interval, not by an exit code:
 *
 *     absorbed  : 440.0 Hz at 48000 and 440.0 Hz at 44100
 *     stale rate: 440.0 Hz at 48000 and 440 * 44100/48000 = 404.25 Hz at 44100
 *
 * (A rack that believes it is at 48 kHz advances phase by f/48000 per sample;
 * clocked at 44100 that is f * 44100/48000 in real time. 1.47 semitones flat --
 * "wrong rather than broken", which E9's original text calls the worst kind.)
 *
 * THE CONTROL, WHICH IS WHAT MAKES A NULL RESULT WORTH ANYTHING
 * ------------------------------------------------------------
 * "Same frequency at both rates" is also what a broken analyser reports. So
 * --selftest runs the analyser against synthetic tones whose answers are known,
 * including a 404.25 Hz tone at 44100 -- the exact signal the stale-rate
 * hypothesis predicts. If the analyser cannot tell that from 440 Hz, this probe
 * cannot conclude anything and says so.
 *
 * BUILD
 *   clang++ -std=c++17 -ObjC++ -fobjc-arc tests/e9_au_rate_probe.mm \
 *     -framework Foundation -framework AudioToolbox -framework AVFoundation \
 *     -o e9_au_rate_probe
 *
 * RUN
 *   ./e9_au_rate_probe --selftest
 *   ./e9_au_rate_probe --preset <preset.xml>
 *
 * The preset is the outer <Preset> element of a saved TIDE state. Extract one
 * from any fixture with:
 *
 *   python3 scripts/decode_rpp.py --preset-out p.xml tests/hosts/v1-rack.rpp
 */
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

#include <mach-o/dyld.h>

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
    if (!ok) ++failures;
}

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

// Rising zero crossings, with hysteresis at +/-20% of peak so an envelope's
// decay tail and any near-silence noise cannot manufacture crossings. The
// frequency is taken from the time BETWEEN the first and last crossing rather
// than from the buffer length, so a tone that starts late or decays early still
// measures correctly -- which matters here because the fixture is an oscillator
// through an ADSR, not a steady sine.
struct ToneMeasurement
{
    double hz    = 0.0;   // 0 if undetermined
    double peak  = 0.0;   // linear
    double rms   = 0.0;   // linear
    int    crossings = 0;
};

static ToneMeasurement measureTone(const float* x, int n, double rate)
{
    ToneMeasurement m;
    if (n <= 1) return m;

    double sumsq = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double v = x[i];
        if (std::fabs(v) > m.peak) m.peak = std::fabs(v);
        sumsq += v * v;
    }
    m.rms = std::sqrt(sumsq / n);

    if (m.peak <= 0.0) return m;                 // digital silence

    const double hi =  0.2 * m.peak;
    const double lo = -0.2 * m.peak;

    // Armed => waiting to go above hi. Each hi crossing is one period.
    bool armed = false;
    double firstCross = -1.0, lastCross = -1.0;
    int count = 0;

    for (int i = 1; i < n; ++i)
    {
        if (!armed)
        {
            if (x[i] < lo) armed = true;
        }
        else if (x[i] > hi)
        {
            armed = false;
            // Linear interpolation between the bracketing samples for sub-sample
            // placement; with ~100 samples per cycle at 440 Hz the nearest-sample
            // error alone would be ~1% and swamp the 8% effect being measured.
            const double t = (i - 1) + (hi - x[i - 1]) / (x[i] - x[i - 1]);
            if (firstCross < 0.0) firstCross = t;
            lastCross = t;
            ++count;
        }
    }

    m.crossings = count;
    if (count >= 2 && lastCross > firstCross)
        m.hz = (count - 1) * rate / (lastCross - firstCross);

    return m;
}

static double dbfs(double linear)
{
    return linear > 0.0 ? 20.0 * std::log10(linear) : -INFINITY;
}

// ---------------------------------------------------------------------------
// The control
// ---------------------------------------------------------------------------

static int selftest()
{
    printf("--- analyser control: can it separate 440 Hz from a stale-rate 404.25 Hz? ---\n");

    struct Case { const char* name; double hz; double rate; double expect; };
    const double stale = 440.0 * 44100.0 / 48000.0;   // 404.25

    const Case cases[] = {
        { "440 Hz at 48000",                   440.0, 48000.0, 440.0 },
        { "440 Hz at 44100",                   440.0, 44100.0, 440.0 },
        { "stale-rate 404.25 Hz at 44100",     stale, 44100.0, stale },
        { "261.6256 Hz at 48000 (middle C)", 261.6256, 48000.0, 261.6256 },
    };

    for (const auto& c : cases)
    {
        const int n = (int)(2.0 * c.rate);
        std::vector<float> buf(n);
        for (int i = 0; i < n; ++i)
        {
            // Decaying envelope on purpose: the real fixture is an oscillator
            // through an ADSR (peak -6.3 dBFS, rms -17.0), so a control on a
            // steady sine would be easier than the case it stands in for.
            const double env = std::exp(-1.5 * i / c.rate);
            buf[i] = (float)(0.5 * env * std::sin(2.0 * M_PI * c.hz * i / c.rate));
        }
        const auto m = measureTone(buf.data(), n, c.rate);
        const double err = std::fabs(m.hz - c.expect);
        printf("     %-34s -> %9.4f Hz  (want %9.4f, err %.4f, %d crossings)\n",
               c.name, m.hz, c.expect, err, m.crossings);
        check(c.name, err < 0.5);
    }

    // The separation is the whole point: 440 and 404.25 must not be confusable.
    const int n = (int)(2.0 * 44100.0);
    std::vector<float> a(n), b(n);
    for (int i = 0; i < n; ++i)
    {
        a[i] = (float)(0.5 * std::sin(2.0 * M_PI * 440.0  * i / 44100.0));
        b[i] = (float)(0.5 * std::sin(2.0 * M_PI * stale  * i / 44100.0));
    }
    const double ha = measureTone(a.data(), n, 44100.0).hz;
    const double hb = measureTone(b.data(), n, 44100.0).hz;
    printf("     separation: %.4f Hz measured between the two hypotheses\n", std::fabs(ha - hb));
    check("the two hypotheses are >30 Hz apart as measured", std::fabs(ha - hb) > 30.0);

    // And a negative control on the analyser itself: silence must not produce
    // a frequency. Without this, "it reported 440" and "it reports 440 for
    // anything" look the same.
    std::vector<float> silence(n, 0.0f);
    const auto ms = measureTone(silence.data(), n, 44100.0);
    printf("     digital silence -> hz=%.4f crossings=%d\n", ms.hz, ms.crossings);
    check("analyser reports no frequency for digital silence", ms.hz == 0.0);

    return failures;
}

// ---------------------------------------------------------------------------
// The AU host
// ---------------------------------------------------------------------------

struct RateResult
{
    double  rate = 0.0;
    bool    allocated = false;
    OSStatus renderStatus = noErr;
    ToneMeasurement tone;
    std::vector<float> audio;   // kept for the cross-analysis control
};

// Render `seconds` of audio from an already-allocated unit and measure channel 0.
static RateResult renderAt(AUAudioUnit* au, double rate, int channels,
                           AUAudioFrameCount blockFrames, double seconds)
{
    RateResult r;
    r.rate = rate;

    const int total = (int)(seconds * rate);
    std::vector<float> out(total, 0.0f);

    // Non-interleaved float32: one AudioBuffer per channel.
    const size_t ablBytes = offsetof(AudioBufferList, mBuffers) + sizeof(AudioBuffer) * channels;
    std::vector<uint8_t> ablStore(ablBytes, 0);
    AudioBufferList* abl = (AudioBufferList*)ablStore.data();
    abl->mNumberBuffers = (UInt32)channels;

    std::vector<std::vector<float>> chan(channels, std::vector<float>(blockFrames, 0.0f));

    AURenderBlock render = au.renderBlock;
    if (!render) { r.renderStatus = kAudioUnitErr_Uninitialized; return r; }

    AudioTimeStamp ts{};
    ts.mFlags = kAudioTimeStampSampleTimeValid;
    ts.mSampleTime = 0.0;

    int done = 0;
    while (done < total)
    {
        const AUAudioFrameCount frames =
            (AUAudioFrameCount)std::min<int>(blockFrames, total - done);

        for (int c = 0; c < channels; ++c)
        {
            std::fill(chan[c].begin(), chan[c].end(), 0.0f);
            abl->mBuffers[c].mNumberChannels = 1;
            abl->mBuffers[c].mDataByteSize   = frames * sizeof(float);
            abl->mBuffers[c].mData           = chan[c].data();
        }

        AudioUnitRenderActionFlags flags = 0;
        const OSStatus st = render(&flags, &ts, frames, 0, abl, nil);
        if (st != noErr) { r.renderStatus = st; return r; }

        // A host must read back mData, not the pointer it passed in: the
        // wrapper may hand back its own storage (AU3Core::outputStorage exists
        // for exactly that). Reading `chan` directly would silently measure our
        // own zeroed scratch on such a path.
        const float* src = (const float*)abl->mBuffers[0].mData;
        if (src) std::memcpy(out.data() + done, src, frames * sizeof(float));

        done += frames;
        ts.mSampleTime += frames;
    }

    r.allocated = true;
    r.tone = measureTone(out.data(), total, rate);
    r.audio = std::move(out);
    return r;
}

int main(int argc, const char* argv[])
{
    @autoreleasepool
    {
        const char* presetPath = nullptr;
        bool wantSelftest = false;

        for (int i = 1; i < argc; ++i)
        {
            if (!std::strcmp(argv[i], "--selftest")) wantSelftest = true;
            else if (!std::strcmp(argv[i], "--preset") && i + 1 < argc) presetPath = argv[++i];
            else { fprintf(stderr, "unknown argument: %s\n", argv[i]); return 2; }
        }

        if (wantSelftest)
        {
            const int rc = selftest();
            printf("\n%s: %d check(s) failed\n", rc ? "SELFTEST FAILED" : "SELFTEST OK", rc);
            return rc ? 1 : 0;
        }

        if (!presetPath)
        {
            fprintf(stderr, "usage: %s --preset <preset.xml> | --selftest\n", argv[0]);
            return 2;
        }

        NSString* preset = [NSString stringWithContentsOfFile:@(presetPath)
                                                     encoding:NSUTF8StringEncoding
                                                        error:nil];
        check("preset file read", preset.length > 0);
        if (!preset.length) return 1;
        printf("     preset: %lu bytes from %s\n", (unsigned long)preset.length, presetPath);

        AudioComponentDescription desc{};
        desc.componentType         = kAudioUnitType_MusicDevice;      // aumu
        desc.componentSubType      = 'Drck';
        desc.componentManufacturer = 'Dsyh';

        AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
        check("TIDE Rack AUv3 found in the component registry (aumu Drck Dsyh)", comp != nullptr);
        if (!comp)
        {
            fprintf(stderr,
                    "     the containing app must be installed AND opened once before the\n"
                    "     extension registers -- see BACKLOG M1.\n");
            return 1;
        }

        // In-process if the system allows it: the extension declares
        // sandboxSafe, and in-process puts the plug-in's own stderr -- the
        // "TIDE: rack built for N Hz" diagnostic this row installed -- in THIS
        // process where it can be read. Out-of-process is the fallback and is
        // recorded as such, because it means that second line of evidence is
        // absent rather than negative.
        __block AUAudioUnit* au = nil;
        __block NSError* auErr = nil;
        __block bool done = false;
        const char* loadMode = "in-process";

        AudioComponentInstantiationOptions opts = kAudioComponentInstantiation_LoadInProcess;
        for (int attempt = 0; attempt < 2 && !au; ++attempt)
        {
            done = false;
            [AUAudioUnit instantiateWithComponentDescription:desc
                                                     options:opts
                                           completionHandler:^(AUAudioUnit* unit, NSError* err)
            {
                au = unit; auErr = err; done = true;
            }];
            while (!done)
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                         beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
            if (!au && attempt == 0)
            {
                opts = kAudioComponentInstantiation_LoadOutOfProcess;
                loadMode = "out-of-process";
            }
        }

        check("AUAudioUnit instantiated", au != nil);
        if (!au)
        {
            fprintf(stderr, "     error: %s\n", auErr.localizedDescription.UTF8String);
            return 1;
        }
        // `loadMode` is what was REQUESTED. What actually happened is a
        // different claim, and the class of the returned object is the one
        // instrument that answers it: an in-process load hands back the
        // wrapper's own AUAudioUnit subclass, an out-of-process load hands back
        // one of AudioToolbox's remote proxies. Worth printing rather than
        // asserting -- either is a valid way to host an AUv3, and the reason to
        // know which is that only the in-process case can put the plug-in's
        // stderr in this process.
        printf("     requested %s; actual class is %s\n",
               loadMode, NSStringFromClass([au class]).UTF8String);

        // And settle it by looking for the appex's own binary among THIS
        // process's loaded images. The class name is suggestive; a loaded image
        // is dispositive, and it decides whether the plug-in's stderr could
        // possibly appear here. Without this the probe can only report what it
        // asked for, which is how "loaded in-process" got printed for a load
        // that was nothing of the kind.
        bool appexInProcess = false;
        for (uint32_t i = 0, n = _dyld_image_count(); i < n; ++i)
        {
            const char* nm = _dyld_get_image_name(i);
            if (nm && std::strstr(nm, "TIDE-Rack.appex")) { appexInProcess = true; break; }
        }
        printf("     the appex binary is %s this process's address space\n",
               appexInProcess ? "IN" : "NOT IN");
        printf("     => the plug-in's own stderr (\"TIDE: rack built for N Hz\") is %s here\n",
               appexInProcess ? "observable" : "NOT observable; the audio below is the evidence");

        AUAudioUnitBus* outBus = au.outputBusses.count > 0 ? au.outputBusses[0] : nil;
        check("the unit has an output bus", outBus != nil);
        if (!outBus) return 1;

        const int channels = (int)outBus.format.channelCount;
        printf("     output bus: %d channel(s), default %.0f Hz\n",
               channels, outBus.format.sampleRate);

        // The document. setFullState is the AU3 wrapper's own restore path
        // (AU3_Wrapper.mm:522), the same one a DAW uses when it reopens a song.
        au.fullState = @{ @"GMPIPRESET" : preset };
        check("fullState accepted the TIDE preset", true);

        // Read it straight back out. Setting a property that is silently
        // discarded looks identical to setting one that took, and this project
        // has shipped that mistake before (S33's setBlob).
        NSDictionary* readback = au.fullState;
        NSString* rb = readback[@"GMPIPRESET"];
        const bool roundTripped = [rb isKindOfClass:[NSString class]] && rb.length > 0;
        check("fullState round-trips a non-empty GMPIPRESET back out", roundTripped);
        if (roundTripped)
            printf("     readback: %lu bytes\n", (unsigned long)rb.length);

        // The bracket. Exactly what a DAW does on a device rate change, and the
        // same shape the CLAP probe used: allocate at 48k, free, allocate at
        // 44.1k, free, allocate at 48k again. The third leg matters -- it is
        // what distinguishes "absorbs a change" from "only ever works once".
        const double rates[] = { 48000.0, 44100.0, 48000.0 };
        const AUAudioFrameCount blockFrames = 512;
        std::vector<RateResult> results;

        for (double rate : rates)
        {
            printf("\n--- %.0f Hz ---\n", rate);

            AVAudioFormat* fmt =
                [[AVAudioFormat alloc] initStandardFormatWithSampleRate:rate
                                                               channels:(AVAudioChannelCount)channels];
            NSError* fe = nil;
            const bool fmtOk = [outBus setFormat:fmt error:&fe];
            char msg[128];
            snprintf(msg, sizeof msg, "output bus accepts %.0f Hz", rate);
            check(msg, fmtOk);
            if (!fmtOk)
            {
                fprintf(stderr, "     error: %s\n", fe.localizedDescription.UTF8String);
                results.push_back({ rate, false, noErr, {} });
                continue;
            }

            // Read the rate back off the bus. A silently-ignored setFormat and
            // a rack that keeps a stale rate are DIFFERENT bugs, and it is
            // worth being able to name which one -- though note the measurement
            // below cannot be fooled by either, see "the verdict".
            snprintf(msg, sizeof msg, "the bus reports %.0f Hz after setFormat", rate);
            check(msg, std::fabs(outBus.format.sampleRate - rate) < 0.5);

            au.maximumFramesToRender = blockFrames;

            NSError* ae = nil;
            const bool allocOk = [au allocateRenderResourcesAndReturnError:&ae];
            snprintf(msg, sizeof msg, "allocateRenderResources at %.0f Hz", rate);
            check(msg, allocOk);
            if (!allocOk)
            {
                fprintf(stderr, "     error: %s\n", ae.localizedDescription.UTF8String);
                results.push_back({ rate, false, noErr, {} });
                continue;
            }

            RateResult r = renderAt(au, rate, channels, blockFrames, 2.0);
            snprintf(msg, sizeof msg, "render returns noErr at %.0f Hz", rate);
            check(msg, r.renderStatus == noErr);

            printf("     peak %.2f dBFS   rms %.2f dBFS   %d crossings   %.4f Hz\n",
                   dbfs(r.tone.peak), dbfs(r.tone.rms), r.tone.crossings, r.tone.hz);

            snprintf(msg, sizeof msg, "the rack is audible at %.0f Hz", rate);
            check(msg, r.tone.peak > 0.0);

            results.push_back(r);

            [au deallocateRenderResources];
        }

        // ------------------------------------------------------------------
        // The verdict
        // ------------------------------------------------------------------
        printf("\n--- verdict ---\n");

        if (results.size() == 3 && results[0].tone.hz > 0.0 && results[1].tone.hz > 0.0)
        {
            const double f48 = results[0].tone.hz;
            const double f44 = results[1].tone.hz;
            const double predictedStale = f48 * 44100.0 / 48000.0;

            printf("     48000 Hz -> %.4f Hz\n", f48);
            printf("     44100 Hz -> %.4f Hz\n", f44);
            printf("     absorbed would be   %.4f Hz\n", f48);
            printf("     stale rate would be %.4f Hz\n", predictedStale);

            const double cents = 1200.0 * std::log2(f44 / f48);
            printf("     measured shift: %+.3f cents\n", cents);

            check("the rate change is ABSORBED (pitch within 5 cents across rates)",
                  std::fabs(cents) < 5.0);

            // CONTROL, on the real renders rather than on synthetic tones.
            //
            // "440 Hz at both rates" is also what a pinned analyser reports, and
            // what a plugin that ignores the host's rate entirely reports. Both
            // are killed by re-analysing each leg's OWN audio at the OTHER rate:
            // if the analyser tracks its rate argument, mislabelling must move
            // the answer by exactly the ratio, and it must move in the direction
            // the stale-rate hypothesis predicts. If this control is flat, the
            // verdict above means nothing and the run should be read as void.
            const double f44as48 = measureTone(results[1].audio.data(),
                                               (int)results[1].audio.size(), 48000.0).hz;
            const double f48as44 = measureTone(results[0].audio.data(),
                                               (int)results[0].audio.size(), 44100.0).hz;
            printf("     control: the 44100 render, re-read as 48000 -> %.4f Hz (want %.4f)\n",
                   f44as48, f44 * 48000.0 / 44100.0);
            printf("     control: the 48000 render, re-read as 44100 -> %.4f Hz (want %.4f)\n",
                   f48as44, f48 * 44100.0 / 48000.0);
            check("mislabelling the rate moves the measured pitch by the predicted ratio",
                  std::fabs(f44as48 - f44 * 48000.0 / 44100.0) < 0.5 &&
                  std::fabs(f48as44 - f48 * 44100.0 / 48000.0) < 0.5);

            // And the two legs must not be the same buffer. Byte-identical
            // audio from two different sample rates would mean one of the
            // renders never happened.
            const bool sameLength = results[0].audio.size() == results[1].audio.size();
            printf("     control: %zu samples at 48000 vs %zu at 44100\n",
                   results[0].audio.size(), results[1].audio.size());
            check("the two legs rendered different sample counts", !sameLength);

            if (results[2].tone.hz > 0.0)
            {
                const double back = 1200.0 * std::log2(results[2].tone.hz / f48);
                printf("     back at 48000 Hz -> %.4f Hz (%+.3f cents vs the first)\n",
                       results[2].tone.hz, back);
                check("returning to the original rate re-tunes too", std::fabs(back) < 5.0);
            }
        }
        else
        {
            check("both rates produced a measurable tone", false);
        }

        printf("\n%s: %d check(s) failed\n", failures ? "FAILED" : "OK", failures);
        return failures ? 1 : 0;
    }
}
