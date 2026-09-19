/* BACKLOG E19 -- a stand-in for SynthEdit_Rack_Adaptor/RackModule.h, used ONLY
 * by tests/e19_datatype_census_probe.sh. It shadows the real header by sitting
 * first on the include path.
 *
 * WHY THIS EXISTS, AND WHY THE FIRST DRAFT OF THE PROBE WAS WRONG WITHOUT IT.
 * TIDE does not compile `modules/X/vcv/X.cpp` -- it compiles `modules/X/X.cpp`,
 * the PORT, which is three lines:
 *
 *     #include "RackModule.h"
 *     #include "vcv/X.cpp"            // upstream, byte-for-byte
 *     RACK_DISPLAY_STATE(&X::member, ...)   // seven of the 39 have one
 *
 * (VCV_Fundamental_gmpi/static_library/CMakeLists.txt:67 -- `${_m}/${_m}.cpp`.)
 *
 * The display-state declaration lives in the PORT, never in upstream's file. A
 * probe that compiles only `vcv/X.cpp` therefore sees no display state for any
 * module and reports `blob=0` across the whole set -- which is not a finding
 * about TIDE, it is the probe measuring the wrong file. That is what the first
 * run of this probe did, and its own control caught it: `blob=0` contradicted a
 * datatype E19 has measured working in a host, so the run went red rather than
 * publishing a clean-looking census of the wrong thing.
 *
 * WHAT THE REAL HEADER DOES that this one does not. RackModule.h is the port's
 * single umbrella include: the Rack mock, the adaptor, the editor, the
 * auto-registration, the generated `RackPluginConfig.h` and the generated panel
 * resources. Everything past the first two needs the GMPI **and** gmpi_ui SDKs
 * plus two CMake-generated headers, i.e. a configured build tree -- which is
 * the one thing this probe is for not needing.
 *
 * WHAT THE PORT ACTUALLY NEEDS FROM IT, for this measurement: the Rack mock (so
 * `vcv/X.cpp` compiles) and RackDisplayState.h (so `RACK_DISPLAY_STATE` is a
 * macro and its registration lands in the same registry `findDisplayState()`
 * reads). Both are `<std>`-only. That is this file.
 *
 * WHAT THAT COSTS IN HONESTY. The census measures the parameters and pins
 * `generatePluginXml()` emits, which depend on the module widget, the menu and
 * the display-state registry -- all of which are present here. It does NOT
 * exercise registration, the editor, or the panel art, and a defect in those
 * would be invisible to it. Anything this probe reports is a statement about
 * what the factory DECLARES, which is what E19's datatype clauses are about.
 */

#pragma once

/* The upstream module sources expect these to have been pulled in already --
 * the real RackModule.h gets them via its own include chain. Listed rather than
 * guessed: <cassert> is WTLFO.cpp:285, <map> is Gates.cpp:48, the rest are
 * cheap insurance. Same list as tests/e82_rack_menu_probe.cpp. */
#include <array>
#include <cassert>
#include <map>
#include <numeric>
#include <random>
#include <set>

#include "rack.hpp"
#include "RackDisplayState.h"
