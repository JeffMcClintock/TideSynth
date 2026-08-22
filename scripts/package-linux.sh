#!/usr/bin/env bash
# R4 - build TIDE-Rack-Linux.tar.gz from an existing Release build tree.
#
# Usage: scripts/package-linux.sh <build-dir> [out-dir]
#
# Naming follows docs/distribution.md: shipped files are dashed (TIDE-Rack),
# never underscored, and never contain a space -- a space becomes %20 in the
# releases/latest/download permalink R6 promises never to change.
set -euo pipefail

BUILD_DIR="${1:?usage: package-linux.sh <build-dir> [out-dir]}"
OUT_DIR="${2:-$BUILD_DIR}"
SRC="$BUILD_DIR/SynthEditSem"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

# NO VERSION IN THIS NAME, DELIBERATELY. I put one here in R10 and it broke the
# v0.1.1 release: release.yml's asset check expects TIDE-Rack-Linux.tar.gz, and
# more importantly release.yml's own header says why the names are version-free
# -- "the version lives in the tag, so releases/latest/download/... is a
# permalink R6 can promise never to change". Two identically-named files across
# releases is the POINT; they are distinguished by their release, not by their
# filename. The version belongs INSIDE the artifacts, which is what the rest of
# R10 does.
PKG="TIDE-Rack-Linux"
ROOT="$STAGE/$PKG"
mkdir -p "$ROOT"

# --- VST3: a self-contained bundle directory. Its resources live INSIDE it
#     (Contents/Resources), so it needs nothing beside it.
[ -d "$SRC/TIDE-Rack.vst3" ] || { echo "error: $SRC/TIDE-Rack.vst3 not found - build the TIDE_Rack_VST3 target first" >&2; exit 1; }
cp -a "$SRC/TIDE-Rack.vst3" "$ROOT/"

# --- CLAP: a BARE shared object on Linux, so it carries no resources of its
#     own. BundleInfo::getBundleContentsFolder() walks the module path for a
#     "Contents" element and falls back to parent_path(), so a bare .clap looks
#     for "Resources" NEXT TO ITSELF -- which is why Resources/ ships alongside
#     and install.sh puts it in ~/.clap. See the README note about that folder
#     being shared between plugins.
[ -f "$SRC/TIDE-Rack.clap" ] || { echo "error: $SRC/TIDE-Rack.clap not found - build the TIDE_Rack_CLAP target first" >&2; exit 1; }
cp -a "$SRC/TIDE-Rack.clap" "$ROOT/"
[ -d "$SRC/Resources" ] || { echo "error: $SRC/Resources not found - the CLAP would load with an empty rack browser" >&2; exit 1; }
cp -a "$SRC/Resources" "$ROOT/"

cat > "$ROOT/install.sh" <<'INSTALL'
#!/usr/bin/env bash
# Install TIDE Rack for the current user. No root, no signing, nothing outside
# your home directory. Re-running it overwrites a previous install.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

VST3_DIR="${VST3_DIR:-$HOME/.vst3}"
CLAP_DIR="${CLAP_DIR:-$HOME/.clap}"

mkdir -p "$VST3_DIR" "$CLAP_DIR"

# VST3 is a self-contained bundle directory.
rm -rf "$VST3_DIR/TIDE-Rack.vst3"
cp -a "$HERE/TIDE-Rack.vst3" "$VST3_DIR/"
echo "installed $VST3_DIR/TIDE-Rack.vst3"

# CLAP is a bare .so and reads its resources from a Resources folder beside it.
cp -a "$HERE/TIDE-Rack.clap" "$CLAP_DIR/"
mkdir -p "$CLAP_DIR/Resources"
cp -a "$HERE/Resources/." "$CLAP_DIR/Resources/"
echo "installed $CLAP_DIR/TIDE-Rack.clap (+ Resources)"

echo
echo "Done. Restart your DAW and rescan plugins."
INSTALL
chmod +x "$ROOT/install.sh"

cat > "$ROOT/README.txt" <<'README'
TIDE Rack - Linux
=================

  ./install.sh

installs, for the current user only:

  ~/.vst3/TIDE-Rack.vst3     the VST3 (a self-contained bundle)
  ~/.clap/TIDE-Rack.clap     the CLAP
  ~/.clap/Resources/         the CLAP's module data - see the note below

Override the destinations with VST3_DIR / CLAP_DIR if your host scans
elsewhere. Nothing is written outside your home directory, and nothing needs
root. To uninstall, delete the paths above.

Note on ~/.clap/Resources
-------------------------
A Linux CLAP is a plain shared object rather than a bundle directory, so it has
nowhere to keep its own data and looks for a "Resources" folder beside itself.
That folder is therefore SHARED with any other CLAP installed the same way. The
VST3 does not have this problem - its resources live inside the bundle.

If you only want the VST3, install it by hand and skip the CLAP:

  cp -a TIDE-Rack.vst3 ~/.vst3/
README

mkdir -p "$OUT_DIR"
TARBALL="$OUT_DIR/$PKG.tar.gz"
rm -f "$TARBALL"
tar -czf "$TARBALL" -C "$STAGE" "$PKG"
echo "$TARBALL"
