#!/usr/bin/env python3
"""Check that every module a shipped prefab uses is one TIDE actually registers.

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

TWO TESTS, BECAUSE ONE WAS NOT ENOUGH -- corrected 2026-08-27 the same day it
shipped, when Jeff photographed the actual dialog and it named a DIFFERENT
module from the one this script had found:

  (a) ABSENT FROM THE BINARY. A registered module id has to exist as a string
      literal somewhere in it, so a type appearing NOWHERE cannot be registered.
      This is what caught `SynthEdit ADSR` in AR_jef.

  (b) DESCRIBED ONLY BY A STAGED XML. TIDE stages ControlsXp.xml,
      MidiPlayer2.xml, Converters.xml and VaFilters.xml to ENRICH the pins of
      modules it already registers -- staging one registers NOTHING. So a type
      whose only presence is a `<Plugin id=>` in one of those files is
      described and absent, which reads as present to test (a). The startup
      line says how bad it is: `ControlsXp.xml enriched 2 of 18 described
      class(es)` -- sixteen described classes TIDE does not have. This is what
      caught `SE Scope3 XP` in Sine_jef, which test (a) scored as fine.

FALSE POSITIVES ARE POSSIBLE UNDER (b) AND NOT UNDER (a): a module that is both
properly registered in C++ AND described in a staged XML would be flagged
wrongly. Measured today that set is empty -- across 14 distinct prefab module
types the two rules together flagged exactly the two real defects and nothing
else -- but it is the direction this script can be wrong in, and a maintainer
who hits it should fix the rule rather than the prefab.

The real authority is what the running app registers, which no offline test can
read. These two are screens, and the honest name for them is screens.

    python3 scripts/check-prefab-modules.py [--binary PATH] [--prefabs DIR]

Exits non-zero when a prefab names a module TIDE does not register. With
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

    # Classes the staged XMLs merely DESCRIBE. Staging an XML enriches the pins
    # of a module TIDE already registers; it registers nothing. So being here
    # and nowhere else means described-but-absent -- see test (b) above.
    described = {}
    for x in sorted(glob.glob(os.path.join(os.path.dirname(binpath), '*.xml'))):
        try:
            xt = open(x, encoding='utf-8', errors='replace').read()
        except OSError:
            continue
        for m in re.finditer(r'<Plugin\s+id="([^"]+)"', xt):
            described.setdefault(m.group(1), os.path.basename(x))
    print('%d class(es) described by %d staged XML(s)'
          % (len(described), len(set(described.values()))))

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
                bad.append((f, t, 'absent from the binary'))
            elif t in described:
                bad.append((f, t, 'described only by %s' % described[t]))

    print('%d prefab(s), %d distinct module type(s) checked' % (len(files), total))
    if bad:
        print()
        print('%d module type(s) used by a shipped prefab that TIDE does not register:'
              % len(bad))
        for f, t, why in bad:
            print('  %-24s %-22s %s' % (os.path.basename(f), t, why))
        print()
        print('A prefab may only use modules TIDE actually links. Either drop the')
        print('module from the prefab, or add it to the compiled-in set -- which is')
        print('a product decision (PLAN constraint 7), not a build fix.')
        return 1

    print('every prefab module type passes both screens, OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
