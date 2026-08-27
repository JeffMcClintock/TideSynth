#!/usr/bin/env bash
#
# BACKLOG R3 -- build TIDE-Rack-macOS.pkg.
#
# Usage:  scripts/package-macos.sh <build-dir> [out-dir]
#
# Produces the asset docs/distribution.md names, installing to the locations
# it specifies:
#
#     TIDE-Rack.vst3        ->  /Library/Audio/Plug-Ins/VST3/
#     TIDE-Rack-AUv3.app    ->  /Applications/
#
# THE AU SHIPS AS AN AUv3, NOT A .component, as of 2026-08-22 (BACKLOG S40,
# Jeff ruling). AU2 and AU3 declare the SAME four-character codes, so macOS
# registers only one and the other is unreachable -- measured: with both
# installed, `auval -a` lists a single `aumu Drck Dsyh` and loads the v3. Rather
# than ship a component no host would reach, TIDE ships the v3 alone. It covers
# Apple's own DAWs; other macOS hosts overwhelmingly take VST3 or CLAP, which
# this pkg and the Linux tarball already carry.
#
# AN AUv3 SHIPS AS AN APP, which is why the payload below is a .app rather than
# a plug-in bundle: the extension lives inside it, and macOS registers the
# extension automatically once the app is in /Applications -- no launch, no
# `pluginkit` call, measured 2026-08-22.
#
# The AU is REQUIRED, not optional. distribution.md's macOS row lists it, and a
# pkg that silently omits half its stated payload is worse than one that fails
# to build -- so this script exits non-zero if the app is missing rather than
# quietly shipping a VST3-only installer. The .gmpi is not shipped to end users
# at all (distribution.md).
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

PRODUCT="TiDE Rack"          # display form  -- docs/distribution.md
ASSET="TIDE-Rack-macOS.pkg"  # shipped form  -- constant, version lives in the tag
IDENTIFIER="com.synthedit.tiderack"
# Kept in step with SynthEdit.cpp's <Plugin version="...">, which is what the
# BUNDLES report. CI sets TIDE_RACK_VERSION from the tag, so this fallback is
# only for a local run -- but it is what shipped when nothing set the variable,
# and it said 0.1.0 while the bundles said 1.0.0.
VERSION="${TIDE_RACK_VERSION:-0.1.1}"

VST3_SRC="$BUILD_DIR/SynthEditSem/TIDE-Rack.vst3"
[ -d "$VST3_SRC" ] || { echo "error: no TIDE-Rack.vst3 in $BUILD_DIR/SynthEditSem" >&2; exit 1; }

AU_SRC="$BUILD_DIR/SynthEditSem/TIDE-Rack-AUv3.app"
if [ ! -d "$AU_SRC" ]; then
    echo "error: no TIDE-Rack-AUv3.app in $BUILD_DIR/SynthEditSem" >&2
    echo "       The AUv3 is part of the shipped payload (docs/distribution.md)." >&2
    echo "       Configure with AU3 in FORMATS_LIST -- SynthEditSem/CMakeLists.txt." >&2
    exit 1
fi

# The extension is the whole point of the app; an app that ships without it
# installs cleanly and provides nothing, which no later check would catch.
# Guarded rather than inlined: under `set -euo pipefail`, a `find` on a missing
# directory fails inside the command substitution and kills the script BEFORE
# the message below can print -- so the guard produced a silent exit 1, which is
# the exact failure mode it exists to prevent. Measured, not theorised.
AU_APPEX=""
if [ -d "$AU_SRC/Contents/PlugIns" ]; then
    AU_APPEX="$(find "$AU_SRC/Contents/PlugIns" -maxdepth 1 -name '*.appex' 2>/dev/null | head -1 || true)"
fi
if [ -z "$AU_APPEX" ]; then
    echo "error: $AU_SRC contains no .appex in Contents/PlugIns." >&2
    echo "       The AUv3 extension is what makes this app worth installing." >&2
    exit 1
fi

# A plist naming an executable that is not there builds and installs fine, and
# macOS silently declines to load it (GMPI#8/#10, issue #271's class). Check the
# two halves agree here, where the artefact is about to be sealed into a pkg.
au_exe="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$AU_APPEX/Contents/Info.plist" 2>/dev/null || true)"
if [ ! -f "$AU_APPEX/Contents/MacOS/$au_exe" ]; then
    echo "error: the appex's CFBundleExecutable is '$au_exe' but Contents/MacOS/$au_exe is missing." >&2
    echo "       macOS will refuse to load this extension. See BACKLOG M3." >&2
    exit 1
fi
echo "==> AUv3 check: appex $(basename "$AU_APPEX"), CFBundleExecutable='$au_exe' matches its binary"

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
mkdir -p "$OUT_DIR/root/Applications"
cp -R "$AU_SRC" "$OUT_DIR/root/Applications/"

# --- sign the payload, if we were given an identity ------------------------
if [ -n "${APPLE_CERTIFICATE_SIGNING_IDENTITY:-}" ]; then
    echo "==> codesign payload"
    # Both bundles, not just the VST3: an unsigned component inside a signed
    # pkg is exactly the shape that passes a casual check and fails Gatekeeper.
    #
    # INSIDE-OUT, AND THE ORDER IS THE FIX. `codesign` without --deep seals only
    # the bundle you name; nested code keeps whatever signature it already had.
    # Every bundle here arrives AD-HOC signed -- `codesign --force --sign -`, in
    # SynthEditSem/CMakeLists.txt's icon-apply step -- so before this the .appex
    # inside the AUv3 app shipped ad-hoc and Apple rejected the whole pkg for it.
    # THREE ERRORS, ONE CAUSE: "not signed with a valid Developer ID certificate",
    # "signature does not include a secure timestamp", and "executable does not
    # have the hardened runtime enabled" are precisely what `--sign -` produces.
    # Notarisation log d3acab2c-68b7-4ead-85ea-d630bb266705, tag v0.1.2, 2026-08-27.
    #
    # The container must come LAST: signing it first and the appex second would
    # break the outer seal. --deep is NOT the fix -- Apple discourages it, and it
    # cannot give a nested bundle its own entitlements.
    AU_DST="$OUT_DIR/root/Applications/TIDE-Rack-AUv3.app"
    AU_DST_APPEX="$AU_DST/Contents/PlugIns/$(basename "$AU_APPEX")"
    if [ ! -d "$AU_DST_APPEX" ]; then
        echo "error: expected the copied appex at $AU_DST_APPEX" >&2
        echo "       The source appex was found, so the copy above changed shape." >&2
        exit 1
    fi

    for bundle in \
        "$OUT_DIR/root/Library/Audio/Plug-Ins/VST3/TIDE-Rack.vst3" \
        "$AU_DST_APPEX" \
        "$AU_DST"
    do
        codesign --force --timestamp --options runtime \
                 --sign "$APPLE_CERTIFICATE_SIGNING_IDENTITY" "$bundle"
        codesign --verify --deep --strict --verbose=2 "$bundle"

        # ASSERT THE AUTHORITY, NOT JUST VALIDITY. The --verify above passed on
        # the ad-hoc appex through two release attempts, and it was right to:
        # ad-hoc IS a valid signature. It simply is not Developer ID, and that
        # distinction is the entirety of what Apple rejected. A check that cannot
        # fail on the bug it stands in front of is not a check.
        #
        # No entitlements exist anywhere in this tree (grepped 2026-08-27), so
        # --force cannot be stripping any. If that changes, this loop needs
        # --entitlements before it needs anything else.
        auth="$(codesign -dvv "$bundle" 2>&1 || true)"
        case "$auth" in
            *"Authority=Developer ID Application"*) ;;
            *)  echo "error: $bundle is not signed by a Developer ID Application authority." >&2
                printf '%s\n' "$auth" | grep -E '^(Authority|Signature|TeamIdentifier)' >&2 || true
                exit 1 ;;
        esac
        case "$auth" in
            *Timestamp=*) ;;
            *)  echo "error: $bundle carries no secure timestamp; Apple will reject it." >&2
                exit 1 ;;
        esac
        echo "    ok  $(basename "$bundle") -- Developer ID, timestamped, hardened runtime"
    done
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
