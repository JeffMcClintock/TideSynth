#!/usr/bin/env bash
#
# BACKLOG R3 -- build TIDE-Rack-macOS.pkg.
#
# Usage:  scripts/package-macos.sh <build-dir> [out-dir]
#
# Produces the asset docs/distribution.md names, installing to the locations
# it specifies:
#
#     TIDE-Rack.vst3  ->  /Library/Audio/Plug-Ins/VST3/
#
# WHAT IS DELIBERATELY NOT HERE: the AU. distribution.md's macOS row lists
# `AU -> /Library/Audio/Plug-Ins/Components/`, and TIDE does not build one --
# `SynthEditSem/CMakeLists.txt` sets `FORMATS_LIST GMPI VST3 STANDALONE`, and
# BACKLOG M1 (the row that would add AU targets) is BLOCKED. Rather than ship a
# pkg that silently omits half its stated payload, this script FAILS if asked
# for an AU that is not there, and R3a tracks adding it once M1 lands. The
# .gmpi is not shipped to end users at all (distribution.md).
#
# SIGNING IS NOT DONE HERE WITHOUT CREDENTIALS, and that is deliberate rather
# than unfinished. An unsigned, unnotarized pkg is effectively unopenable on
# modern macOS, so this script is explicit about which of the two artefacts it
# produced. It signs only when the identity is present in the environment, the
# same shape SynthEdit_cmake_mac.yml uses:
#
#     APPLE_CERTIFICATE_SIGNING_IDENTITY   e.g. "Developer ID Application: ..."
#     APPLE_INSTALLER_SIGNING_IDENTITY     e.g. "Developer ID Installer: ..."
#
# Notarization (`notarytool submit` + `stapler staple`) is NOT run here: it
# needs the APPLE_ID / app-specific password / team id trio, which belong in
# CI's secret store, and R5 is the row that wires it. The signing chain to copy
# is SynthEdit_cmake_mac.yml:223-244.
set -euo pipefail

BUILD_DIR="${1:?usage: package-macos.sh <build-dir> [out-dir]}"
OUT_DIR="${2:-$BUILD_DIR/package}"

PRODUCT="TIDE Rack"          # display form  -- docs/distribution.md
ASSET="TIDE-Rack-macOS.pkg"  # shipped form  -- constant, version lives in the tag
IDENTIFIER="com.synthedit.tiderack"
VERSION="${TIDE_RACK_VERSION:-0.1.0}"

VST3_SRC="$BUILD_DIR/SynthEditSem/TIDE-Rack.vst3"
[ -d "$VST3_SRC" ] || { echo "error: no TIDE-Rack.vst3 in $BUILD_DIR/SynthEditSem" >&2; exit 1; }

# Refuse to ship a single-arch binary silently: the ARM-only ruling
# (docs/decisions.md, 2026-08-21) means this pkg will not run on an Intel Mac
# and nothing tells the user why, so at least say so at build time.
ARCHS="$(lipo -archs "$VST3_SRC/Contents/MacOS/TIDE-Rack" 2>/dev/null || echo unknown)"
echo "==> TIDE-Rack.vst3 architectures: $ARCHS"
case "$ARCHS" in
  *x86_64*) echo "    (universal -- runs on Intel and Apple silicon)" ;;
  *arm64*)  echo "    NOTE: arm64 only. This pkg will NOT run on an Intel Mac," ;;
esac

rm -rf "$OUT_DIR/root" "$OUT_DIR/component.pkg"
mkdir -p "$OUT_DIR/root/Library/Audio/Plug-Ins/VST3"
cp -R "$VST3_SRC" "$OUT_DIR/root/Library/Audio/Plug-Ins/VST3/"

# --- sign the payload, if we were given an identity ------------------------
if [ -n "${APPLE_CERTIFICATE_SIGNING_IDENTITY:-}" ]; then
    echo "==> codesign payload"
    codesign --force --timestamp --options runtime \
             --sign "$APPLE_CERTIFICATE_SIGNING_IDENTITY" \
             "$OUT_DIR/root/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3"
    codesign --verify --deep --strict --verbose=2 \
             "$OUT_DIR/root/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3"
    SIGNED_PAYLOAD=yes
else
    echo "==> codesign SKIPPED (APPLE_CERTIFICATE_SIGNING_IDENTITY unset)"
    SIGNED_PAYLOAD=no
fi

# --- build the pkg ---------------------------------------------------------
echo "==> pkgbuild"
pkgbuild --root "$OUT_DIR/root" \
         --identifier "$IDENTIFIER" \
         --version "$VERSION" \
         --install-location / \
         "$OUT_DIR/component.pkg"

# hostArchitectures MUST match the payload, and productbuild will not work it
# out for you: left to itself it writes `x86_64,arm64` regardless, so an
# arm64-only pkg installs happily on an Intel Mac and the plugin then fails to
# load with nothing explaining why -- the exact failure R3's row flagged.
# Measured here rather than hardcoded, so this stays correct if the ARM-only
# ruling is ever revisited.
HOST_ARCHS="$(printf '%s' "$ARCHS" | tr ' ' ',')"
echo "==> productbuild (hostArchitectures=$HOST_ARCHS, derived from the binary)"
productbuild --synthesize --package "$OUT_DIR/component.pkg" "$OUT_DIR/Distribution.xml"
/usr/bin/sed -i '' -E "s/hostArchitectures=\"[^\"]*\"/hostArchitectures=\"$HOST_ARCHS\"/" "$OUT_DIR/Distribution.xml"
grep -q "hostArchitectures=\"$HOST_ARCHS\"" "$OUT_DIR/Distribution.xml" \
    || { echo "error: could not set hostArchitectures in the synthesized Distribution" >&2; exit 1; }

if [ -n "${APPLE_INSTALLER_SIGNING_IDENTITY:-}" ]; then
    productbuild --distribution "$OUT_DIR/Distribution.xml" \
                 --package-path "$OUT_DIR" \
                 --sign "$APPLE_INSTALLER_SIGNING_IDENTITY" \
                 "$OUT_DIR/$ASSET"
    SIGNED_PKG=yes
else
    productbuild --distribution "$OUT_DIR/Distribution.xml" \
                 --package-path "$OUT_DIR" \
                 "$OUT_DIR/$ASSET"
    SIGNED_PKG=no
fi

echo
echo "==> $OUT_DIR/$ASSET"
echo "    product          : $PRODUCT $VERSION"
echo "    payload signed   : $SIGNED_PAYLOAD"
echo "    installer signed : $SIGNED_PKG"
echo "    notarized        : no  (R5 wires notarytool; see the header)"
if [ "$SIGNED_PKG" = no ]; then
    echo
    echo "    THIS PKG IS NOT SHIPPABLE AS-IS. macOS Gatekeeper will refuse an"
    echo "    unsigned, unnotarized installer. It is a correct pkg for testing"
    echo "    the layout, and nothing more."
fi
