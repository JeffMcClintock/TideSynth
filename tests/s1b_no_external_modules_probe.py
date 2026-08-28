#!/usr/bin/env python3
"""BACKLOG S1b -- the shipped binary carries no third-party module loader.

THE ACCEPT, as corrected by the row's addendum C1 and note (3):

  * none of the binary-loader family is exported: ScanBundle,
    ScanStandaloneSem, ScanPluginBinary, ScanFile, UnloadDll, ReloadDll,
    SemCacheName, LoadModuleData, StoreModuleData, ClearModuleDataCache,
    Module_Info3::LoadDllOnDemand, MP_DllLoad, LoadOrScanModuleData;
  * dlopen / dlsym / dlclose are NOT imported;
  * dladdr IS still imported -- BundleInfo finds the plugin's own bundle with
    it, and C1 exists because a run nearly chased it as a fourth dl* symbol;
  * ScanFolder IS still exported -- the prefab scanner stays (save-as-prefab
    rescans the prefab folder through it). Its presence doubles as the control
    that this probe is looking at a real editor-carrying binary rather than
    passing vacuously against the wrong file.

Exit codes: 0 pass - 1 the loader is back (or the keep-list broke) - 2 setup.
"""
import argparse, pathlib, re, subprocess, sys

FORBIDDEN = ["ScanBundle(", "ScanStandaloneSem(", "ScanPluginBinary(", "ScanFile(",
             "UnloadDll(", "ReloadDll(", "SemCacheName()", "LoadModuleData()",
             "StoreModuleData()", "ClearModuleDataCache()",
             "Module_Info3::LoadDllOnDemand", "MP_DllLoad", "LoadOrScanModuleData"]
REQUIRED_EXPORT = ["ScanFolder("]
FORBIDDEN_IMPORTS = ["_dlopen", "_dlsym", "_dlclose"]
REQUIRED_IMPORTS = ["_dladdr"]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True, help="the shipped plugin binary (Mach-O)")
    args = ap.parse_args()
    b = pathlib.Path(args.binary)
    if not b.is_file():
        print(f"SETUP FAILED -- no such file: {b}"); return 2
    try:
        exports = subprocess.run(["nm", "-gU", str(b)], capture_output=True, text=True, check=True).stdout
        exports = subprocess.run(["c++filt"], input=exports, capture_output=True, text=True).stdout
        imports = subprocess.run(["nm", "-u", str(b)], capture_output=True, text=True, check=True).stdout
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"SETUP FAILED -- {e}"); return 2

    fails = []
    for s in FORBIDDEN:
        if s in exports:
            fails.append(f"exported (loader is back): {s}")
    for s in REQUIRED_EXPORT:
        if s not in exports:
            fails.append(f"NOT exported (wrong binary, or the prefab scanner broke): {s}")
    imp = set(imports.split())
    for s in FORBIDDEN_IMPORTS:
        if s in imp:
            fails.append(f"imported: {s}")
    for s in REQUIRED_IMPORTS:
        if s not in imp:
            fails.append(f"NOT imported (dladdr must stay -- BundleInfo needs it): {s}")

    print(f"S1b -- no third-party loader in {b.name}")
    if fails:
        for f in fails: print(f"  FAIL  {f}")
        return 1
    print(f"  PASS  {len(FORBIDDEN)} loader symbols absent, ScanFolder present, "
          f"dlopen/dlsym/dlclose gone, dladdr kept")
    return 0

if __name__ == "__main__":
    sys.exit(main())
