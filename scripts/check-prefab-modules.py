#!/usr/bin/env python3
"""Check that every module a shipped prefab uses is actually IN the binary.

BACKLOG E48. `check-prefab-layout.py` already checks that every
`<module type=X>` has a `<Plugin id=X>` in the file's own `PluginList` -- but a
prefab carries its own PluginList, so that asks whether the FILE describes its
modules, not whether the PRODUCT has them. `AR_jef.synthedit` passed that check
for weeks while using `SynthEdit ADSR`, which TIDE neither compiles nor stages
XML for.

What that costs, measured 2026-08-27 (windows):

  - inserting the prefab appears to work and the rack looks right,
  - the saved document therefore contains `type="SynthEdit ADSR"`,
  - on reload `CContainer::ImportChildren` cannot resolve it and raises
    `SeMessageBoxAsync("Module not found in factory: ...", L"", MB_OK)`
    (CContainer.cpp:1089) -- a BLOCKING MessageBoxW with an EMPTY caption,
    during `SessionState::restore`, before `-quiet` is applied and before the
    command channel exists. The app hangs with no window title, no stderr and
    ~0.08s of CPU, and only a human clicking OK releases it,
  - the module is then skipped, so its connectors are dropped and the
    "Connectors lost while loading" dialog follows (CContainer.cpp:1143).

So a shipped prefab makes a saved project unloadable, which is PLAN's v0.1
clause "the patch survives save-and-reload" failing on a shipped asset.

THE TEST IS ABSENCE, DELIBERATELY. A registered module id has to exist as a
string literal in the binary, so a type string that appears NOWHERE in it
cannot possibly be registered under that id. The converse is not true -- a
string can be present for other reasons -- so this reports only the direction
it can be sure of, and stays silent on the rest. That is the safe way to be
wrong: a new prefab using a module nobody linked fails, and no correct prefab
is ever failed by a coincidence.

    python3 scripts/check-prefab-modules.py [--binary PATH] [--prefabs DIR]

Exits non-zero when a prefab names a module the binary does not contain. With
no binary found it SKIPS and says so -- a check that silently passes because it
could not run is the failure mode this repo keeps re-learning.
"""
import argparse
import glob
import os
import re
import sys

# Types that are structural rather than modules, or are parameter/datatype
# names sharing the `type=` attribute. None of these is looked up in the
# factory by ImportChildren, so none belongs in this test.
NOT_MODULES = {
    'Line', 'Container',
    'bool', 'enum', 'float', 'int', 'midi', 'string_utf8', 'blob', 'double',
}

MODULE_RE = re.compile(r'<module\s[^>]*type="([^"]+)"')


def binary_candidates(explicit):
    if explicit:
        return [explicit]
    pats = [
        'build/SynthEditSem/Release/TIDE-Rack.exe',
        'build/SynthEditSem/TIDE-Rack.app/Contents/MacOS/TIDE-Rack',
        'build/SynthEditSem/TIDE-Rack',
    ]
    out = []
    for p in pats:
        out.extend(glob.glob(p))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--binary', default=None)
    ap.add_argument('--prefabs', default='RackModules')
    ap.add_argument('--repo-root', default='.')
    args = ap.parse_args()

    root = args.repo_root
    os.chdir(root)

    cands = [p for p in binary_candidates(args.binary) if os.path.isfile(p)]
    if not cands:
        print('SKIP -- no TIDE-Rack binary found; nothing was checked.')
        print('       pass --binary PATH, or build first. This is a skip, not a pass.')
        return 0
    binpath = cands[0]
    blob = open(binpath, 'rb').read()
    print('binary: %s (%d bytes)' % (binpath, len(blob)))

    def present(name):
        return (blob.count(name.encode('utf-8')) > 0
                or blob.count(name.encode('utf-16-le')) > 0)

    files = sorted(glob.glob(os.path.join(args.prefabs, '*.synthedit')))
    if not files:
        print('SKIP -- no prefabs under %s; nothing was checked.' % args.prefabs)
        return 0

    # A control, so a broken reader cannot report a clean sweep: these ship in
    # every rack and MUST be found. If they are not, the test itself is wrong.
    controls = ['SE TiDE:Panel', 'TiDE Patch Point In', 'SE Label']
    missing_controls = [c for c in controls if not present(c)]
    if missing_controls:
        print('FAIL -- the control modules are not in the binary either: %s'
              % ', '.join(missing_controls))
        print('        that means this check cannot read this binary, not that '
              'the prefabs are wrong.')
        return 2

    bad = []
    total = 0
    for f in files:
        try:
            text = open(f, encoding='utf-8').read()
        except OSError as e:
            print('FAIL -- cannot read %s: %s' % (f, e))
            return 2
        types = sorted({t for t in MODULE_RE.findall(text) if t not in NOT_MODULES})
        total += len(types)
        for t in types:
            if not present(t):
                bad.append((f, t))

    print('%d prefab(s), %d distinct module type(s) checked' % (len(files), total))
    if bad:
        print()
        print('%d module type(s) used by a shipped prefab and ABSENT from the binary:'
              % len(bad))
        for f, t in bad:
            print('  %-34s %s' % (os.path.basename(f), t))
        print()
        print('A prefab may only use modules TIDE actually links. Either drop the')
        print('module from the prefab, or add it to the compiled-in set -- which is')
        print('a product decision (PLAN constraint 7), not a build fix.')
        return 1

    print('every prefab module type is present in the binary, OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
