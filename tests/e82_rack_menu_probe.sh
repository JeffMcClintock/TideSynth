#!/usr/bin/env bash
# Build and run tests/e82_rack_menu_probe.cpp -- BACKLOG E82.
#
# Needs two sibling checkouts and a C++20 compiler. Nothing is configured, no
# CMake runs, and neither checkout is written to:
#
#   $1  SynthEdit_Rack_Adaptor   (default ../SynthEdit_Rack_Adaptor)
#   $2  VCV_Fundamental_gmpi     (default ../VCV_Fundamental_gmpi)
#
# Windows: run from a VS x64 developer prompt, or with cl on PATH -- the same
# convention tests/e80_clap_feedback_probe.c states. mac/linux: uses $CXX, else
# c++.
#
#   bash tests/e82_rack_menu_probe.sh
#
# Exit 0 means at least one module offers a context-menu option AND every model
# linked. Exit 1 means either none do (E82's assertion as filed) or the build is
# short a TU -- the output says which.
#
# 39 translation units, about a minute with MSVC. Object files go to a temp dir
# that is deleted on exit; neither checkout is touched.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
ADAPTOR=${1:-$(cd "$here/../.." && pwd)/SynthEdit_Rack_Adaptor}
VCV=${2:-$(cd "$here/../.." && pwd)/VCV_Fundamental_gmpi}

for d in "$ADAPTOR/RackPanelLayout.h" "$VCV/modules"; do
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

PROBE=$(nat "$here/e82_rack_menu_probe.cpp")
INCS="-I$(nat "$ADAPTOR") -I$(nat "$ADAPTOR/rack") -I$(nat "$ADAPTOR/compat")"

OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
cd "$OUT"

# -w: the module sources are VCV's, byte for byte, and their warnings are not
# this probe's business.
compile() { # <output-base> <define-or-empty> <extra-include>
    local base=$1 def=$2 inc=$3
    if [ "$WIN" = 1 ]; then
        cl -nologo -std:c++20 -EHsc -w -DRACK_NO_AUTO_REGISTER ${def:+"$def"} \
           $INCS "$inc" -c "$PROBE" -Fo"$base.obj" >/dev/null
    else
        "$CXX" -std=c++20 -w -DRACK_NO_AUTO_REGISTER ${def:+"$def"} \
           $INCS "$inc" -c "$PROBE" -o "$base.o"
    fi
}

for m in $MODULES; do
    safe=$(echo "$m" | tr -c 'A-Za-z0-9' '_')
    compile "tu_$safe" "-DE82_MODULE_SOURCE=\"$(nat "$VCV/modules/$m/vcv/$m.cpp")\"" \
            "-I$(nat "$VCV/modules/$m/vcv")"
done

# main() takes any one module's plugin.hpp -- they are copies of one file, and
# it is the file that declares all 39 models.
compile main "" "-I$(nat "$VCV/modules/WTLFO/vcv")"

if [ "$WIN" = 1 ]; then
    link -nologo -OUT:e82probe.exe ./*.obj >/dev/null
    ./e82probe.exe
else
    "$CXX" ./*.o -o e82probe
    ./e82probe
fi
