/* BACKLOG E82 -- does a VCV rack module offer ANY context-menu option of its
 * own, and if so which modules and which options?
 *
 * E82 was filed 2026-09-07 (macos) with the headline "A RIGHT-CLICK ON A RACK
 * MODULE'S PANEL RETURNS THE RACK'S OWN CONTEXT MENU, NOT THE MODULE'S", and
 * read that as E19's `int/bool/enum` clause "having no producer at all". The
 * five probe points that produced it were all on the VCV **Scope**.
 *
 * THIS PROBE READS THE EXACT DATA THE RIGHT-CLICK MENU IS BUILT FROM, for every
 * module in TIDE's compiled-in VCV set, with no editor, no window and no host.
 *
 *   SynthEdit_Rack_Adaptor/RackEditor.h:128   layout = readPanelLayout(*model)
 *   SynthEdit_Rack_Adaptor/RackEditor.h:633   if (layout.menu.empty())
 *                                                 return ReturnCode::Unhandled;
 *
 * `layout.menu` is the whole input to RackEditor::populateContextMenu: empty
 * means the module adds nothing and the menu the user sees is the rack's own,
 * byte for byte. So "is there anything to toggle on this module" is answerable
 * by calling readPanelLayout() and looking -- which is what this does.
 *
 * WHAT IT IS NOT. It does not exercise the host-side routing
 * (ViewBase::populateContextMenu -> ModuleView::populateContextMenu), and it
 * does not click anything. It answers WHICH MODULES HAVE OPTIONS; it does not
 * answer whether a right-click reaches them. Those are two questions and this
 * is deliberately only the first -- see the E82 row for the second.
 *
 * HOW IT AVOIDS NEEDING THE GMPI SDK. readPanelLayout() lives in
 * RackPanelLayout.h, whose only include is the adaptor's rack.hpp mock, whose
 * own includes are all <std>. Registration is what pulls in GMPI, and
 * RACK_NO_AUTO_REGISTER -- the adaptor's own documented opt-out
 * (RackAutoRegister.h) -- turns it off. So the whole probe is the module
 * sources plus two headers, and builds in about a minute with nothing
 * configured.
 *
 * ONE FILE, TWO ROLES, selected by E82_MODULE_SOURCE:
 *
 *   -DE82_MODULE_SOURCE="<abs path to modules/X/vcv/X.cpp>"
 *       compile that module's upstream source into its own TU, exactly as
 *       VCV_Fundamental_gmpi/modules/X/X.cpp does, minus RackModule.h.
 *   (undefined)
 *       compile main(), which walks every model and prints its menu table.
 *
 * tests/e82_rack_menu_probe.sh generates the per-module command lines, builds,
 * links and runs. Read it for the exact invocation.
 *
 * LICENSING. The BUILT probe links VCV Fundamental's sources and is therefore
 * GPL-3.0-or-later, like every other consumer of that repo. This file is TIDE's
 * and is ISC; it is a developer instrument and is never shipped.
 */

#ifdef E82_MODULE_SOURCE

/* The upstream module sources expect these to have been pulled in already --
 * the real build gets them via RackModule.h, which this probe does not use
 * because it would drag in the GMPI SDK. Listed rather than guessed: <cassert>
 * is WTLFO.cpp:285, <map> is Gates.cpp:48, the rest are cheap insurance. */
#include <array>
#include <cassert>
#include <map>
#include <numeric>
#include <random>
#include <set>

#include E82_MODULE_SOURCE

#else /* ---------------------------------------------------------------- */

#include <cassert>
#include <cstddef>
#include <cstdio>

#include "RackPanelLayout.h"
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

const char* kindName(rack_adaptor::MenuOptionKind k)
{
    switch (k)
    {
    case rack_adaptor::MenuOptionKind::Separator: return "SEP";
    case rack_adaptor::MenuOptionKind::BoolPtr:   return "BOOL";
    default:                                      return "INDEX";
    }
}

} // namespace

int main()
{
    int withOptions = 0, total = 0, missing = 0;

    for (const auto& e : entries)
    {
        ++total;
        if (!e.model || !*e.model)
        {
            /* A null model means the TU was not linked in -- a build error, not
             * a finding about the module. Counted separately for that reason. */
            std::printf("%-18s MODEL NULL (not linked)\n", e.name);
            ++missing;
            continue;
        }

        const auto layout = rack_adaptor::readPanelLayout(**e.model);

        int options = 0;
        for (const auto& m : layout.menu)
            if (m.kind != rack_adaptor::MenuOptionKind::Separator)
                ++options;

        std::printf("%-18s options=%d entries=%zu\n", e.name, options, layout.menu.size());

        for (const auto& m : layout.menu)
        {
            std::printf("    %-5s %-28s default=%d labels=%zu",
                        kindName(m.kind), m.name.c_str(), m.defaultValue, m.labels.size());
            if (!m.labels.empty())
            {
                std::printf("  [");
                for (std::size_t i = 0; i < m.labels.size(); ++i)
                    std::printf("%s%s", i ? "," : "", m.labels[i].c_str());
                std::printf("]");
            }
            std::printf("\n");
        }

        if (options)
            ++withOptions;
    }

    std::printf("\nMODULES WITH AT LEAST ONE ADAPTOR-MODELLED OPTION: %d of %d\n",
                withOptions, total);
    if (missing)
        std::printf("MODELS NOT LINKED: %d -- the build is incomplete, not the answer\n", missing);

    /* Non-zero if nothing has options at all, which is the state E82 asserted;
     * a green exit is the finding that the assertion does not hold. */
    return (withOptions > 0 && missing == 0) ? 0 : 1;
}

#endif
