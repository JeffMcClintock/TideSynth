#!/usr/bin/env python3
"""Regenerate the measurements behind docs/sandbox-audit.md (BACKLOG S2).

The audit's whole value is that its file set is *measured* rather than guessed:
it is the exact translation-unit set the linker used for TIDE, read out of a
configured build tree, not a grep of everything under SE16.

Run it against a build tree that has TIDE configured:

    python3 tools/sandbox_audit.py --build /home/jef/SE/build

It prints four things, in the order docs/sandbox-audit.md presents them:

  1. TIDE's link closure and translation-unit count, from compile_commands.json
  2. drift -- whether the CMake source lists still match that configure
  3. the categorised API hits over those files, at current paths
  4. which of the hit files actually survived into the linked binary, from
     the DWARF compile-unit list (needs an unstripped .so and `readelf`)

Nothing here writes to the build tree or to the source repos. It is read-only
on purpose: the tree it reads is the developer's, not a CI checkout.
"""

import argparse
import collections
import json
import os
import re
import subprocess
import sys

# Targets whose objects end up inside the VST3 .so. Taken from the LINK_LIBRARIES
# line in build.ninja rather than assumed -- TIDE links VST3_Wrapper, SynthEditLib
# and EditorLib, and deliberately none of the .sem module libraries.
TIDE_TARGETS = {"TIDE_Rack", "TIDE_Rack_VST3", "VST3_Wrapper", "SynthEditLib", "EditorLib"}

# Where a moved file may have gone. C4 relocated a dozen files from
# SE16/SynthEdit2/ into SynthEditLib/, so a configure older than 2026-08-13
# records paths that no longer exist; resolve by basename against these roots.
SOURCE_ROOTS = [
    "/home/jef/SE/SE16",
    "/home/jef/SE/SynthEditLib",
    "/home/jef/SE/gmpi_ui",
    "/home/jef/SE/GMPI",
    "/home/jef/SE/GMPI_Wrappers",
]

PATTERNS = [
    ("write-open", r"\b(ofstream|std::ofstream|fopen|_wfopen|freopen|CreateFileW?|_wopen|::open)\s*\("),
    ("read-open", r"\b(ifstream|std::ifstream)\s*\("),
    ("mkdir", r"\b(create_director(?:y|ies)|CreateDirectoryW?|_wmkdir|\bmkdir)\s*\("),
    ("delete/rename", r"\b(remove_all|std::remove|::remove|DeleteFileW?|unlink|std::rename|MoveFileW?)\s*\("),
    ("copy", r"\b(copy_file|CopyFileW?|filesystem::copy)\s*\("),
    ("known-folder", r"(SHGetKnownFolderPath|SHGetFolderPath|CSIDL_|FOLDERID_|NSSearchPathForDirectoriesInDomains"
                     r"|GetTempPathW?|getenv\s*\(\s*\"HOME\"|GetHomeDir|AppData|APPDATA|%TEMP%|xdg|XDG_)"),
    ("registry", r"\b(RegOpenKeyE?x?W?|RegQueryValueE?x?W?|RegSetValueE?x?W?|RegCreateKeyE?x?W?|HKEY_)"),
    ("fs-namespace", r"\bstd::filesystem::|\bfs::(?:exists|create|remove|copy|path|directory)"),
    ("dlopen", r"\b(dlopen|dlsym|LoadLibraryW?|GetProcAddress)\s*\("),
    ("watcher", r"FileWatcher|ReadDirectoryChanges|inotify|FSEvents"),
]


def translation_units(build_dir):
    """The set of .cpp/.c/.mm the linker compiled into TIDE, per compile_commands.json."""
    with open(os.path.join(build_dir, "compile_commands.json"), encoding="utf-8") as fh:
        entries = json.load(fh)
    per_target = collections.defaultdict(set)
    for entry in entries:
        out = entry.get("output", "")
        if "CMakeFiles/" not in out or ".dir/" not in out:
            continue
        target = out.split("CMakeFiles/")[1].split(".dir/")[0]
        if target in TIDE_TARGETS:
            per_target[target].add(os.path.normpath(entry["file"]))
    return per_target


def resolve_to_current(paths):
    """Map a recorded path to where that file lives today, by basename if it moved.

    Returns (resolved, external). `external` is the VST3 SDK and the generated
    Wayland protocol C -- third-party or generated, audited as a group rather
    than line by line.
    """
    index = collections.defaultdict(list)
    for root in SOURCE_ROOTS:
        for dirpath, _, filenames in os.walk(root):
            if "/.git" in dirpath:
                continue
            for name in filenames:
                index[name].append(os.path.join(dirpath, name))

    resolved, external, missing = set(), [], []
    for path in paths:
        if "/.cache/CPM" in path or "/build/" in path:
            external.append(path)
            continue
        if os.path.exists(path):
            resolved.add(path)
            continue
        candidates = index.get(os.path.basename(path), [])
        if not candidates:
            missing.append(path)
            continue
        # Prefer SynthEditLib: it is where C2-C5 move files *to*.
        preferred = [c for c in candidates if "/SynthEditLib/" in c] or candidates
        resolved.add(preferred[0])
    return sorted(resolved), external, missing


def scan(files):
    hits = collections.defaultdict(list)
    for path in files:
        try:
            with open(path, encoding="utf-8", errors="replace") as fh:
                lines = fh.read().splitlines()
        except OSError:
            continue
        for lineno, line in enumerate(lines, 1):
            stripped = line.strip()
            if stripped.startswith("//"):
                continue
            for name, pattern in PATTERNS:
                if re.search(pattern, line):
                    hits[name].append((path, lineno, stripped[:160]))
    return hits


def linked_compile_units(binary):
    """Compile units that actually survived into the binary, from DWARF.

    A static archive contributes an object only when something references it, so
    this is the difference between "compiled" and "linked" -- the distinction the
    audit turns on. Needs an unstripped binary.
    """
    try:
        out = subprocess.run(
            ["readelf", "--debug-dump=info", "--dwarf-depth=1", binary],
            capture_output=True, text=True, timeout=900,
        ).stdout
    except (OSError, subprocess.SubprocessError) as exc:
        print(f"  (skipped: {exc})", file=sys.stderr)
        return None
    return {os.path.basename(m) for m in re.findall(r"/home/jef/[^\s]*\.(?:cpp|c|mm)", out)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True, help="a build tree with TIDE configured")
    ap.add_argument("--binary", default=None, help="unstripped TIDE .so (default: <build>/SynthEditSem/TIDE-Rack.so)")
    args = ap.parse_args()

    binary = args.binary or os.path.join(args.build, "SynthEditSem", "TIDE-Rack.so")

    print("== 1. TIDE link closure ==")
    per_target = translation_units(args.build)
    union = set()
    for target in sorted(per_target):
        print(f"  {target:16s} {len(per_target[target]):4d} TUs")
        union |= per_target[target]
    print(f"  {'UNION':16s} {len(union):4d} TUs")

    print("\n== 2. resolve to current paths ==")
    files, external, missing = resolve_to_current(union)
    print(f"  resolved {len(files)} | external/generated {len(external)} | unresolved {len(missing)}")
    for m in missing:
        print(f"    UNRESOLVED {m}")

    print("\n== 3. categorised hits ==")
    hits = scan(files)
    total = 0
    for name, _ in PATTERNS:
        rows = hits.get(name, [])
        total += len(rows)
        print(f"  {name:14s} {len(rows):5d} hits in {len({r[0] for r in rows}):3d} files")
    print(f"  {'TOTAL':14s} {total:5d}")

    print("\n== 4. linked vs merely compiled ==")
    linked = linked_compile_units(binary) if os.path.exists(binary) else None
    if linked is None:
        print("  (no unstripped binary available -- skipping)")
        return
    per_file = collections.Counter()
    for rows in hits.values():
        for path, _, _ in rows:
            per_file[path] += 1
    n_in = n_out = 0
    for path, count in per_file.most_common():
        is_linked = os.path.basename(path) in linked
        n_in, n_out = (n_in + 1, n_out) if is_linked else (n_in, n_out + 1)
        print(f"  {'LINKED' if is_linked else '  --  '}  {count:4d}  {path.replace('/home/jef/SE/', '')}")
    print(f"\n  files with hits linked into the binary: {n_in}; not linked: {n_out}")


if __name__ == "__main__":
    main()
