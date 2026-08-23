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

# --- CLAP: a "semi-bundle" -- the .clap and its Resources inside one directory
#     named for the product, installed as ~/.clap/TIDE-Rack/ (S37).
#
#     WHY THIS SHAPE, because it is not the obvious one and the obvious one is
#     wrong. CLAP has no bundle format on Linux: clap/entry.h is explicit that
#     "plugin_path is the path to the DSO (Linux, Windows), or the bundle
#     (macOS)", so the macOS trick of hiding resources inside TIDE-Rack.clap/
#     Contents/ does not work here -- MEASURED in REAPER 7.43, which discovered
#     every other layout tried and did NOT discover that one.
#
#     What DOES work is the same header's search rule: each directory is
#     "recursively searched" for files ending in .clap. So a plugin may live in
#     its own subdirectory and still be found -- REAPER finds
#     ~/.clap/TIDE-Rack/TIDE-Rack.clap and lists it as "TIDE Rack (TIDE Synth)".
#
#     That subdirectory is what buys us encapsulation for free.
#     BundleInfo::getBundleContentsFolder() walks the module path for a
#     "Contents" element, finds none, and returns parent_path() -- which is now
#     ~/.clap/TIDE-Rack/ rather than ~/.clap/. So Resources resolves INSIDE our
#     own directory with NO code change in SynthEditLib. Verified by strace:
#     every read (the four XMLs, all six prefabs, skins) resolves under
#     TIDE-Rack/Resources/, and the plugin reports "6 rack prefab(s) seeded".
#
#     WHAT IT FIXES: previously Resources/ landed directly in ~/.clap, shared
#     with every other CLAP installed the same way. Two GMPI-based CLAPs would
#     overwrite each other's same-named files, and uninstalling either would
#     delete files the other was by then relying on. One directory per product
#     removes the shared namespace entirely, and makes uninstall "rm -rf" of a
#     single folder.
CLAP_BUNDLE="$ROOT/TIDE-Rack"
mkdir -p "$CLAP_BUNDLE"
[ -f "$SRC/TIDE-Rack.clap" ] || { echo "error: $SRC/TIDE-Rack.clap not found - build the TIDE_Rack_CLAP target first" >&2; exit 1; }
cp -a "$SRC/TIDE-Rack.clap" "$CLAP_BUNDLE/"
[ -d "$SRC/Resources" ] || { echo "error: $SRC/Resources not found - the CLAP would load with an empty rack browser" >&2; exit 1; }
cp -a "$SRC/Resources" "$CLAP_BUNDLE/"

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

# CLAP ships as a directory ("semi-bundle"): the .clap plus its own Resources.
# Hosts search the CLAP folders recursively, so a plugin in its own subfolder is
# still found, and its resources stay out of everyone else's way.
rm -rf "$CLAP_DIR/TIDE-Rack"
cp -a "$HERE/TIDE-Rack" "$CLAP_DIR/"
echo "installed $CLAP_DIR/TIDE-Rack/ (plug-in + Resources)"

# Older versions installed the .clap loose in $CLAP_DIR with its Resources in
# the SHARED $CLAP_DIR/Resources. Remove the stale plug-in, or the host would
# list TIDE Rack twice and the old copy would still read the shared folder.
if [ -f "$CLAP_DIR/TIDE-Rack.clap" ]; then
  rm -f "$CLAP_DIR/TIDE-Rack.clap"
  echo "removed the previous $CLAP_DIR/TIDE-Rack.clap"
fi

# $CLAP_DIR/Resources is deliberately NOT deleted. By now it may hold files that
# belong to another plug-in -- deleting a shared folder by name is the exact
# failure this release stops doing. TIDE no longer reads it; remove it yourself
# if nothing else uses it.
if [ -d "$CLAP_DIR/Resources" ]; then
  echo "note: $CLAP_DIR/Resources is left alone - TIDE no longer uses it."
  echo "      Delete it yourself if no other plug-in needs it."
fi

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
  ~/.clap/TIDE-Rack/         the CLAP and its module data, in one folder

Override the destinations with VST3_DIR / CLAP_DIR if your host scans
elsewhere. Nothing is written outside your home directory, and nothing needs
root.

To uninstall, delete those two paths. Each is a single self-contained folder.

Why the CLAP is a folder
------------------------
CLAP has no bundle format on Linux, so a plug-in cannot hide its data inside
the .clap file the way a macOS bundle does. But hosts search their CLAP folders
RECURSIVELY, so a plug-in may sit in a subfolder of its own and still be found.

TIDE Rack uses that: it ships as ~/.clap/TIDE-Rack/ containing the plug-in and
its Resources. Everything it needs is inside one folder that belongs to it.

Earlier versions put the plug-in loose in ~/.clap and its data in a SHARED
~/.clap/Resources. That was a bad idea: two plug-ins packaged that way overwrite
each other's files, and uninstalling one can delete files the other still needs.
If you have that older layout, install.sh removes the stale plug-in for you. It
does NOT delete ~/.clap/Resources, because those files may no longer be ours --
delete it yourself if no other plug-in uses it.

If you only want the VST3, install it by hand and skip the CLAP:

  cp -a TIDE-Rack.vst3 ~/.vst3/
README

mkdir -p "$OUT_DIR"
TARBALL="$OUT_DIR/$PKG.tar.gz"
rm -f "$TARBALL"
tar -czf "$TARBALL" -C "$STAGE" "$PKG"
echo "$TARBALL"
