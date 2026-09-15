/* BACKLOG E19 -- which GMPI datatypes does the rack adaptor actually put on the
 * wire, and in which direction, for every module TIDE compiles in?
 *
 * E19's Accept lists five datatype shapes for the DSP<->GUI path and disposes of
 * them unevenly: **float** (a light) and **blob** (a display-state frame) are
 * measured and passing on this platform; **int/bool/enum** is "verified by
 * toggling a VCV context-menu option"; and **string** has carried the same
 * sentence since 2026-08-25 --
 *
 *     "**string** has no current producer -- note that in the row rather than
 *      inventing one."
 *
 * That is an assertion, not a measurement, and it is the only clause of this row
 * that has never had an artifact behind it. THIS PROBE MEASURES IT, and it gets
 * the other four for free, because they all come out of one function.
 *
 * WHAT IT READS. `rack_adaptor::generatePluginXml()` -- the real one,
 * SynthEdit_Rack_Adaptor/RackAdaptor.h:187 -- is the SINGLE place a rack
 * module's pins and parameters come into existence. RackAutoRegister.h:127
 * calls it and nothing else does; the host scans the plugin and gets what it
 * returns. So the set of datatypes this path can carry IS the set of
 * `datatype="..."` strings that function emits, and the per-module counts are
 * what it emits for that module. Calling it for all 39 models and tallying the
 * result is the whole measurement -- no plug-in, no host, no window, no build
 * tree.
 *
 * WHY THAT IS THE RIGHT QUESTION rather than a proxy for it. A "producer" for
 * E19's purposes is a pin the DSP can write and the editor can read. In the
 * emitted XML that is exactly an `<Audio>` pin carrying `direction="out"` and
 * `private="true"` (RackAdaptor.h:374-385) -- lights and the display-state blob.
 * This probe counts those separately from everything else, so "no string
 * producer" is a count of a named thing rather than a failure to find one.
 *
 * THE SECOND HALF OF THE ANSWER IS A COMPILE ERROR, NOT A COUNT.
 * tests/e19_string_member_negative_control.cpp shows the display-state channel
 * REFUSES a std::string member at compile time (RackDisplayState.h:94, the
 * trivially-copyable static_assert, which names std::string in its own text).
 * The census says no module offers a string today; the negative control says
 * the one channel that carries bytes could not be made to, which is the
 * stronger statement and the one that stops this being re-derived every few
 * weeks.
 *
 * WHAT IT IS NOT. It does not create an editor, click anything, or exercise
 * routing -- E82's arm and E19's `int/bool/enum` clause still need that. It
 * reads what the factory declares, which is upstream of every one of those.
 *
 * HOW IT AVOIDS NEEDING A BUILD TREE. `RackAdaptor.h` needs the GMPI SDK for
 * `Processor.h` and `Extensions/PinConnection.h`, and nothing else -- two -I
 * flags at the GMPI checkout. `-DRACK_NO_AUTO_REGISTER` (the adaptor's own
 * opt-out, RackAutoRegister.h) keeps registration, and with it the rest of the
 * SDK, out of the module TUs.
 *
 * IT COMPILES THE PORT, NOT UPSTREAM'S FILE, AND THAT DISTINCTION IS THE WHOLE
 * DIFFERENCE BETWEEN THIS AND tests/e82_rack_menu_probe.cpp. TIDE builds
 * `modules/X/X.cpp`; `RACK_DISPLAY_STATE` is declared there and nowhere else,
 * so a probe that compiles `modules/X/vcv/X.cpp` measures `blob=0` for every
 * module in the set. tests/e19-shim/RackModule.h is what lets the port compile
 * without a build tree, and explains itself at length.
 *
 * ONE FILE, TWO ROLES, selected by E19_MODULE_SOURCE -- the same shape as
 * tests/e82_rack_menu_probe.cpp, deliberately:
 *
 *   -DE19_MODULE_SOURCE="<abs path to modules/X/X.cpp>"
 *       compile that module's PORT into its own TU.
 *   (undefined)
 *       compile main(), which walks every model and prints the census.
 *
 * tests/e19_datatype_census_probe.sh builds, links and runs it. Read that for
 * the exact invocation.
 *
 * LICENSING. The BUILT probe links VCV Fundamental's sources and is therefore
 * GPL-3.0-or-later, like every other consumer of that repo. This file is TIDE's
 * and is ISC; it is a developer instrument and is never shipped.
 */

#ifdef E19_MODULE_SOURCE

/* The port's own first line is `#include "RackModule.h"`, which resolves to
 * tests/e19-shim/RackModule.h here -- that is where the std prelude and the two
 * headers a port needs come from. Nothing else is required at this level. */
#include E19_MODULE_SOURCE

#else /* ---------------------------------------------------------------- */

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#include "RackAdaptor.h"
#include "plugin.hpp"   /* any one module's copy: it declares all 39 models */

namespace {

struct Entry { const char* name; Model** model; };

/* The 39 models behind the 38 module directories -- SequentialSwitch registers
 * two. Order follows static_library/CMakeLists.txt's `_rack_static_modules`,
 * which is the list that decides what TIDE actually links. */
const Entry entries[] = {
    {"8vert", &model_8vert},          {"ADSR", &modelADSR},
    {"Compare", &modelCompare},       {"CVMix", &modelCVMix},
    {"Delay", &modelDelay},           {"Fade", &modelFade},
    {"Gates", &modelGates},           {"LFO", &modelLFO},
    {"Logic", &modelLogic},           {"Merge", &modelMerge},
    {"MidSide", &modelMidSide},       {"Mixer", &modelMixer},
    {"Mult", &modelMult},             {"Mutes", &modelMutes},
    {"Noise", &modelNoise},           {"Octave", &modelOctave},
    {"Process", &modelProcess},       {"Pulses", &modelPulses},
    {"Push", &modelPush},             {"Quantizer", &modelQuantizer},
    {"Random", &modelRandom},         {"RandomValues", &modelRandomValues},
    {"Rescale", &modelRescale},       {"Scope", &modelScope},
    {"SEQ3", &modelSEQ3},             {"SequentialSwitch1", &modelSequentialSwitch1},
    {"SequentialSwitch2", &modelSequentialSwitch2}, {"SHASR", &modelSHASR},
    {"Split", &modelSplit},           {"Sum", &modelSum},
    {"Unity", &modelUnity},           {"VCA-1", &modelVCA_1},
    {"VCA", &modelVCA},               {"VCF", &modelVCF},
    {"VCMixer", &modelVCMixer},       {"VCO", &modelVCO},
    {"Viz", &modelViz},               {"WTLFO", &modelLFO2},
    {"WTVCO", &modelVCO2},
};

/* The emitted XML is built by string concatenation, one element per line, so a
 * line-oriented reader is exact here rather than approximate. Attributes are
 * always `name="value"` with the value escaped by detail::appendEscaped, which
 * never emits a bare quote. */
std::string attr(const std::string& line, const char* name)
{
    const std::string key = std::string(name) + "=\"";
    const auto at = line.find(key);
    if (at == std::string::npos)
        return {};
    const auto from = at + key.size();
    const auto to = line.find('"', from);
    return to == std::string::npos ? std::string{} : line.substr(from, to - from);
}

struct Census
{
    /* Keyed BY THE DATATYPE STRING so an unexpected one shows up as itself
     * rather than falling into an "other" bucket. That is the point: a census
     * that could only count the types it already knew about would answer E19's
     * string question by construction. */
    std::map<std::string, int> parameters;   // <Parameters> declarations
    std::map<std::string, int> guiPins;      // <GUI> pins that name a datatype
    std::map<std::string, int> audioIn;      // <Audio> pins, no direction="out"
    std::map<std::string, int> audioOut;     // <Audio> direction="out", public
    std::map<std::string, int> dspToGui;     // <Audio> direction="out" private="true"

    void merge(const Census& o)
    {
        for (const auto& kv : o.parameters) parameters[kv.first] += kv.second;
        for (const auto& kv : o.guiPins)    guiPins[kv.first]    += kv.second;
        for (const auto& kv : o.audioIn)    audioIn[kv.first]    += kv.second;
        for (const auto& kv : o.audioOut)   audioOut[kv.first]   += kv.second;
        for (const auto& kv : o.dspToGui)   dspToGui[kv.first]   += kv.second;
    }
};

Census censusOf(const std::string& xml)
{
    Census c;
    int section = 0;   // 1 Parameters, 2 GUI, 3 Audio

    std::size_t pos = 0;
    for (;;)
    {
        const auto nl = xml.find('\n', pos);
        if (nl == std::string::npos)
            break;
        const std::string line = xml.substr(pos, nl - pos);
        pos = nl + 1;

        if (line.find("<Parameters>") != std::string::npos) { section = 1; continue; }
        if (line.find("<GUI")         != std::string::npos) { section = 2; continue; }
        if (line.find("<Audio>")      != std::string::npos) { section = 3; continue; }

        const std::string type = attr(line, "datatype");
        if (type.empty())
            continue;   /* a <Pin parameterId=".."/> inherits its parameter's type */

        const bool out  = line.find("direction=\"out\"") != std::string::npos;
        const bool priv = line.find("private=\"true\"")  != std::string::npos;

        if (section == 1)      c.parameters[type]++;
        else if (section == 2) c.guiPins[type]++;
        else if (section == 3)
        {
            if (!out)      c.audioIn[type]++;
            else if (priv) c.dspToGui[type]++;
            else           c.audioOut[type]++;
        }
    }
    return c;
}

std::string brief(const std::map<std::string, int>& m)
{
    if (m.empty())
        return "-";
    std::string s;
    for (const auto& kv : m)
    {
        if (!s.empty()) s += " ";
        s += kv.first + "=" + std::to_string(kv.second);
    }
    return s;
}

int count(const std::map<std::string, int>& m, const char* t)
{
    const auto it = m.find(t);
    return it != m.end() ? it->second : 0;
}

} // namespace

int main(int argc, char** argv)
{
    /* --xml <Model> dumps one module's generated XML verbatim. The census is a
     * summary of exactly this text, and a reviewer should be able to see the
     * thing being summarised without rebuilding anything. */
    const char* dumpOnly = nullptr;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], "--xml") == 0)
            dumpOnly = argv[i + 1];

    Census total;
    int missing = 0, models = 0;

    if (!dumpOnly)
        std::printf("%-18s %-24s %-14s %-12s %s\n",
                    "MODULE", "PARAMETERS", "GUI PINS", "AUDIO IN",
                    "DSP->GUI (out+private)");

    for (const auto& e : entries)
    {
        if (!e.model || !*e.model)
        {
            /* A null model means the TU was not linked in -- a build error, not
             * a finding about the module. Counted separately for that reason. */
            if (!dumpOnly)
                std::printf("%-18s MODEL NULL (not linked)\n", e.name);
            ++missing;
            continue;
        }

        if (dumpOnly && std::strcmp(dumpOnly, e.name) != 0)
            continue;

        /* The same four options RackAutoRegister.h fills in before calling the
         * generator, so the XML measured here is the XML the host is handed. */
        const std::string id = std::string("VCV: ") + e.name;
        rack_adaptor::RegistrationOptions opt;
        opt.id       = id.c_str();
        opt.name     = e.name;
        opt.category = "Rack/VCV";
        opt.vendor   = "VCV (ported)";

        const auto xml = rack_adaptor::generatePluginXml(**e.model, opt);

        if (dumpOnly)
        {
            std::fputs(xml.c_str(), stdout);
            return 0;
        }

        const auto c = censusOf(xml);
        total.merge(c);
        ++models;

        std::printf("%-18s %-24s %-14s %-12s %s\n",
                    e.name, brief(c.parameters).c_str(), brief(c.guiPins).c_str(),
                    brief(c.audioIn).c_str(), brief(c.dspToGui).c_str());
    }

    if (dumpOnly)
    {
        std::fprintf(stderr, "no such model: %s\n", dumpOnly);
        return 2;
    }

    std::printf("\n%d model(s) measured\n", models);
    std::printf("  <Parameters> declared    : %s\n", brief(total.parameters).c_str());
    std::printf("  <GUI> pins naming a type : %s\n", brief(total.guiPins).c_str());
    std::printf("  <Audio> in               : %s\n", brief(total.audioIn).c_str());
    std::printf("  <Audio> out, public      : %s\n", brief(total.audioOut).c_str());
    std::printf("  <Audio> out, private     : %s   <-- the DSP->GUI feedback path\n",
                brief(total.dspToGui).c_str());

    const int strings = count(total.parameters, "string") + count(total.guiPins, "string")
                      + count(total.audioIn, "string")    + count(total.audioOut, "string")
                      + count(total.dspToGui, "string");

    /* E19 names five shapes. `int` is not one of the XML's words: the adaptor
     * emits `enum` for an IndexPtr option and the EDITOR holds it in a
     * Pin<int32_t> (RackEditor.h:1184), so the row's "int/bool/enum" is two
     * datatypes on the wire, not three. */
    std::printf("\nE19's datatype clauses, off the census above:\n");
    std::printf("  float  : %d parameter(s), %d on the DSP->GUI path (lights)\n",
                count(total.parameters, "float"), count(total.dspToGui, "float"));
    std::printf("  blob   : %d parameter(s), %d on the DSP->GUI path (display state)\n",
                count(total.parameters, "blob"), count(total.dspToGui, "blob"));
    std::printf("  bool   : %d parameter(s), %d on the DSP->GUI path\n",
                count(total.parameters, "bool"), count(total.dspToGui, "bool"));
    std::printf("  enum   : %d parameter(s), %d on the DSP->GUI path\n",
                count(total.parameters, "enum"), count(total.dspToGui, "enum"));
    std::printf("  string : %d anywhere, in %d module(s) -- %s\n", strings, models,
                strings ? "A PRODUCER EXISTS; E19's row is out of date"
                        : "no producer, measured rather than asserted");

    if (missing)
        std::printf("\nMODELS NOT LINKED: %d -- the build is incomplete, not the answer\n",
                    missing);

    /* Green means the measurement ran AND its own controls held: every model
     * linked, and the four datatypes E19 has already measured are all present,
     * so a zero for `string` is an absence inside data that demonstrably
     * contains other things rather than an empty census. Red if any of that
     * moves -- including a string producer appearing, which would make this
     * row's standing sentence wrong and is what a later reader needs told. */
    const bool controlsHeld =
        count(total.parameters, "float") > 0 && count(total.parameters, "bool") > 0 &&
        count(total.parameters, "enum")  > 0 && count(total.dspToGui, "blob")   > 0 &&
        count(total.dspToGui, "float")   > 0;

    if (!controlsHeld)
        std::printf("\nCONTROL FAILED: the census is missing a datatype E19 has already "
                    "measured -- do not read the string result off this run\n");

    return (missing == 0 && controlsHeld && strings == 0) ? 0 : 1;
}

#endif
