#!/usr/bin/env python3
"""BACKLOG S21 — does the build tree actually satisfy the resource reader?

Replicates BundleInfo::getBundleContentsFolder (SynthEditLib/modules/se_sdk3_hosting/
BundleInfo.cpp:204-218) and the Linux branch of getResourceFolder (:296-299), then
asks whether TideApp::seedPrefabsFromBundle would find anything. The staging side
and the reading side are written in different repos and disagreed silently on Linux
until 2026-08-20; this asserts they agree, from the reader's point of view.

    python3 tests/s21_bundle_resources_probe.py <build-dir>

Exit 0 if both formats seed. Run it against a PRE-fix tree as a positive control:
it must FAIL, or it is not measuring anything. Linux-only — the path algorithm
branches per platform and this encodes the Linux answer."""
import sys, pathlib

def bundle_contents_folder(module_path: pathlib.Path) -> pathlib.Path:
    acc = pathlib.Path(module_path.anchor)
    for part in module_path.parts:
        acc = acc / part
        if part == "Contents":
            return acc
    return module_path.parent          # "we're not in a bundle"

def resource_folder(module_path):      # Linux branch
    return bundle_contents_folder(module_path) / "Resources"

def check(label, module_path):
    p = pathlib.Path(module_path)
    if not p.exists():
        print(f"  {label:<12} MODULE MISSING {p}"); return False
    res = resource_folder(p)
    prefabs = res / "Prefabs"
    n_pre = len(list(prefabs.glob("*.synthedit"))) if prefabs.is_dir() else 0
    n_xml = len(list(res.glob("*.xml"))) if res.is_dir() else 0
    ok = prefabs.is_dir() and n_pre > 0
    print(f"  {label:<12} reader looks in: {res}")
    print(f"  {'':<12} -> Prefabs dir: {'YES' if prefabs.is_dir() else 'NO'}  "
          f"prefabs={n_pre} xml={n_xml}  => "
          f"{'seeds ok' if ok else 'EMPTY BROWSER (seedPrefabsFromBundle returns early)'}")
    return ok

root = pathlib.Path(sys.argv[1])
label = sys.argv[2]
print(f"\n=== {label} ===")
a = check("TIDE_VST3", root/"SynthEditSem/TIDE_VST3.vst3/Contents/x86_64-linux/TIDE_VST3.so")
b = check("TIDE.gmpi", root/"SynthEditSem/TIDE.gmpi")
print(f"  RESULT: {'PASS' if (a and b) else 'FAIL'}")
sys.exit(0 if (a and b) else 1)
