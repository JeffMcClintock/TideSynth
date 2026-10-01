#!/usr/bin/env bash
# Build and run tests/e19_datatype_census_probe.cpp -- BACKLOG E19's datatype
# clauses, `string` in particular.
#
# Needs three sibling checkouts and a C++20 compiler. Nothing is configured, no
# CMake runs, and no checkout is written to:
#
#   $1  SynthEdit_Rack_Adaptor   (default ../SynthEdit_Rack_Adaptor)
#   $2  VCV_Fundamental_gmpi     (default ../VCV_Fundamental_gmpi)
#   $3  GMPI                     (default ../GMPI)
#
# The GMPI checkout is the one thing tests/e82_rack_menu_probe.sh does not need.
# readPanelLayout() is reachable from the Rack mock alone; generatePluginXml()
# is not, because RackAdaptor.h includes Processor.h and
# Extensions/PinConnection.h. Two -I flags is the whole cost -- it is still
# header-only from here, and nothing links against a built GMPI.
#
# Windows: run from a VS x64 developer prompt, or with cl on PATH -- the same
# convention tests/e82_rack_menu_probe.sh states. mac/linux: uses $CXX, else c++.
#
#   bash tests/e19_datatype_census_probe.sh
#   bash tests/e19_datatype_census_probe.sh '' '' '' --xml WTLFO   # one module's XML
#
# Exit 0 means: every model linked, the census found the four datatypes E19 has
# already measured (float, bool, enum, blob), it found NO string anywhere, and
# the display-state channel refused a std::string member at compile time while
# accepting the same construction without it. Exit 1 means one of those moved --
# the output says which. Exit 2 is a missing checkout.
#
# 41 translation units, about a minute with MSVC. Object files go to a temp dir
# that is deleted on exit; no checkout is touched.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
siblings=$(cd "$here/../.." && pwd)
ADAPTOR=${1:-}; ADAPTOR=${ADAPTOR:-$siblings/SynthEdit_Rack_Adaptor}
VCV=${2:-};     VCV=${VCV:-$siblings/VCV_Fundamental_gmpi}
GMPI=${3:-};    GMPI=${GMPI:-$siblings/GMPI}
shift $(( $# > 3 ? 3 : $# )) || true

for d in "$ADAPTOR/RackAdaptor.h" "$VCV/modules" "$GMPI/Core/Processor.h" \
         "$GMPI/Extensions/PinConnection.h"; do
    [ -e "$d" ] || { echo "missing: $d" >&2; exit 2; }
done

# The same list as VCV_Fundamental_gmpi/static_library/CMakeLists.txt's
# _rack_static_modules -- what TIDE actually links, not what the repo holds.
MODULES="8vert ADSR Compare CVMix Delay Fade Gates LFO Logic Merge MidSide
Mixer Mult Mutes Noise Octave Process Pulses Push Quantizer Random RandomValues
Rescale Scope SEQ3 SequentialSwitch SHASR Split Sum Unity VCA-1 VCA VCF VCMixer
VCO Viz WTLFO WTVCO"

# MSVC accepts `-` for every flag and forward slashes in every path, which is
# what keeps this one code path on all three platforms -- `/Fo` under MSYS gets
# rewritten into a filesystem path, and `cygpath -m` keeps drive letters.
if command -v cl >/dev/null 2>&1; then
    WIN=1; nat() { cygpath -m "$1"; }
else
    WIN=0; CXX=${CXX:-c++}; nat() { printf '%s' "$1"; }
fi

PROBE=$(nat "$here/e19_datatype_census_probe.cpp")
CONTROL=$(nat "$here/e19_string_member_negative_control.cpp")

# The shim directory comes FIRST and the order is load-bearing: it holds a
# RackModule.h that shadows the adaptor's, which is what lets a PORT compile
# with no build tree. See tests/e19-shim/RackModule.h for why the port and not
# upstream's file is the thing to compile.
INCS="-I$(nat "$here/e19-shim")
      -I$(nat "$ADAPTOR") -I$(nat "$ADAPTOR/rack") -I$(nat "$ADAPTOR/compat")
      -I$(nat "$GMPI") -I$(nat "$GMPI/Core")"

OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
cd "$OUT"

# -w: the module sources are VCV's, byte for byte, and their warnings are not
# this probe's business.
compile() { # <output-base> <source> <define-or-empty> <extra-include-or-empty>
    local base=$1 src=$2 def=$3 inc=$4
    if [ "$WIN" = 1 ]; then
        cl -nologo -std:c++20 -EHsc -w -DRACK_NO_AUTO_REGISTER ${def:+"$def"} \
           $INCS ${inc:+"$inc"} -c "$src" -Fo"$base.obj" >/dev/null
    else
        "$CXX" -std=c++20 -w -DRACK_NO_AUTO_REGISTER ${def:+"$def"} \
           $INCS ${inc:+"$inc"} -c "$src" -o "$base.o"
    fi
}

for m in $MODULES; do
    safe=$(echo "$m" | tr -c 'A-Za-z0-9' '_')
    # modules/$m/$m.cpp -- the PORT, which is what static_library/CMakeLists.txt
    # compiles and the only place RACK_DISPLAY_STATE is declared. Its own
    # `#include "vcv/$m.cpp"` resolves relative to the port's directory, so no
    # -I is needed for it.
    compile "tu_$safe" "$PROBE" \
            "-DE19_MODULE_SOURCE=\"$(nat "$VCV/modules/$m/$m.cpp")\"" ""
done

# main() takes any one module's plugin.hpp -- they are copies of one file, and
# it is the file that declares all 39 models.
compile main "$PROBE" "" "-I$(nat "$VCV/modules/WTLFO/vcv")"

if [ "$WIN" = 1 ]; then
    # Linked through `cl`, not `link`, and the spelling is load-bearing: Git
    # Bash ships a coreutils link at /usr/bin/link.exe, which comes first on
    # PATH when bash is launched from a VS developer cmd rather than the other
    # way round -- and spelling it `link.exe` does not dodge it, because the
    # coreutils one IS a .exe. It answers `link: unknown option -- n`, which
    # reads as a bad flag rather than as the wrong program entirely. `cl` has no
    # such twin and drives the real linker itself.
    cl -nologo ./*.obj -Fe:e19probe.exe >/dev/null
    RUN=./e19probe.exe
else
    "$CXX" ./*.o -o e19probe
    RUN=./e19probe
fi

rc=0
"$RUN" "$@" || rc=$?

# --xml is a dump, not a measurement; the controls below would say nothing
# about it.
for a in "$@"; do [ "$a" = "--xml" ] && exit "$rc"; done

echo
echo "NEGATIVE CONTROL -- the display-state channel and a std::string member"

# Arm 1: the same construction WITHOUT the string must compile. Without this,
# a file that failed for a typo would read as evidence.
if compile control_ok "$CONTROL" "-DE19_OMIT_STRING" "" 2>/dev/null; then
    echo "  trivially-copyable member only : COMPILES (expected)"
else
    echo "  trivially-copyable member only : FAILED TO COMPILE -- the control file"
    echo "                                   is broken; arm 2 proves nothing"
    rc=1
fi

# Arm 2: adding the std::string must NOT compile.
if compile control_string "$CONTROL" "" "" 2>/dev/null; then
    echo "  plus a std::string member      : COMPILES -- THE GUARD IS GONE."
    echo "                                   RackDisplayState.h's trivially-copyable"
    echo "                                   static_assert no longer refuses a string;"
    echo "                                   E19's string clause needs re-reading"
    rc=1
else
    echo "  plus a std::string member      : REFUSED at compile time (expected)"
    echo "                                   RackDisplayState.h:94, the static_assert"
    echo "                                   whose own text names std::string"
fi

exit "$rc"
