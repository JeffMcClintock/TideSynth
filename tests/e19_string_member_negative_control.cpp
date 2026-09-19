/* BACKLOG E19 -- the negative control for the `string` clause.
 *
 * THIS FILE MUST NOT COMPILE. tests/e19_datatype_census_probe.sh builds it and
 * fails if it SUCCEEDS.
 *
 * The census in tests/e19_datatype_census_probe.cpp counts what the 39
 * compiled-in modules declare today, and finds no string anywhere. On its own
 * that is an absence, and an absence is only ever a statement about the sample.
 * This file turns it into a statement about the CHANNEL: the display-state blob
 * is the one pin on the DSP->GUI path that carries arbitrary bytes, and it is
 * the only place a string could plausibly arrive -- so if it refuses a
 * std::string at COMPILE time, no module can add one later without the refusal
 * being noticed.
 *
 * It does refuse, at SynthEdit_Rack_Adaptor/RackDisplayState.h:94, whose
 * static_assert names std::string in its own message:
 *
 *     "RACK_DISPLAY_STATE: every member must be trivially copyable. A member
 *      that owns memory (std::string, std::vector) cannot be copied as bytes
 *      between two module instances - send what it points AT instead."
 *
 * The control is worth having as a build step rather than as a citation because
 * a static_assert is the kind of guard that gets relaxed in passing. If someone
 * loosens it, this file starts compiling and the probe goes red on the same run
 * that the census stops being the whole answer.
 *
 * WHAT IT DOES NOT SHOW. It does not show that a string is impossible by some
 * other route -- a future adaptor could emit `datatype="string"` for something
 * that is not display state. That half is the census's job, and the two are
 * only worth reading together.
 *
 * Built with the same flags as the module TUs (-DRACK_NO_AUTO_REGISTER), so it
 * needs no GMPI SDK either.
 */

#include <string>

#include "rack.hpp"
#include "RackDisplayState.h"

namespace {

struct StringStateModule : rack::Module
{
    /* Trivially copyable, and accepted -- present so a failure here is
     * attributable to the string member rather than to the module shape. */
    float fine = 0.f;

    /* The member the channel must refuse. */
    std::string label;
};

} // namespace

/* Two arms, so a compile failure is attributable to the STRING rather than to
 * anything else about this file. The .sh builds both and requires the first to
 * fail and the second to succeed -- a file that fails to compile for a typo
 * would otherwise pass as evidence.
 *
 * Expected: error at RackDisplayState.h:94 (the trivially-copyable
 * static_assert). Anything that compiles is the finding. */
#ifdef E19_OMIT_STRING
RACK_DISPLAY_STATE(&StringStateModule::fine)
#else
RACK_DISPLAY_STATE(&StringStateModule::fine, &StringStateModule::label)
#endif
