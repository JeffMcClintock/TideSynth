#!/usr/bin/env python3
"""Census a GMPI <Preset>: parameters, module types, and patch cables.

    python3 scripts/dump_preset.py <preset.xml> [<preset.xml> ...]

A <Preset> is what `gmpiController.getPreset()` returns, what a VST3 instance
stores, what the AU3 wrapper carries as its GMPIPRESET key, and what the CLAP
wrapper writes to `clap.state`. `scripts/decode_rpp.py --preset-out` extracts
one from a REAPER project; `tests/e69_clap_state_probe.c` writes one straight
out of a plug-in with no host involved.

WHY THIS EXISTS RATHER THAN A SIZE COMPARISON (BACKLOG E69)
-----------------------------------------------------------
Save/restore work is judged on whether the patch survived, and the tempting
instrument is the byte count. It is not good enough in either direction:

  * A save that RE-MINTS the document produces different bytes for the same
    rack -- 14,136 in, 13,930 out, with every module and cable intact. Read as
    a size, that is a loss; read as a census, it is a re-serialisation.
  * A save that echoes its input produces identical bytes, and so does a
    correct mint fed its own previous output, because the mint is a fixed
    point. So a size MATCH distinguishes nothing at all.

A DIFFERENCE in size does prove one thing -- the save did not echo its input --
and that is the whole of what E68 read off the .als. Everything else needs the
counts below.

The nesting is three deep and is why a plain grep for "Cable" finds nothing:
each <Param> holds a base64 document, and each of that document's <patch-list>
<s> entries holds ANOTHER base64 document, which is where <Cable> lives.
"""
import base64
import collections
import re
import sys


def _b64(text):
    try:
        return base64.b64decode(text)
    except Exception:
        return b''


def params(preset_text):
    """Yield (id, decoded bytes) for every <Param> carrying a payload."""
    for m in re.finditer(r'<Param\s+id="(\d+)"\s+val="([^"]*)"', preset_text):
        if not m.group(2):
            continue
        raw = _b64(m.group(2))
        if raw:
            yield int(m.group(1)), raw


def cables(document_bytes):
    """Every <Cable .../> element, from inside the patch-list's base64 blobs."""
    found = []
    for s in re.finditer(rb'<s>([A-Za-z0-9+/=]{20,})</s>', document_bytes):
        inner = _b64(s.group(1).decode())
        if inner:
            found += re.findall(rb'<Cable\b[^>]*/>', inner)
    return found


def module_types(document_bytes):
    return collections.Counter(
        re.findall(rb'<Module\b[^>]*\bType="([^"]*)"', document_bytes))


def report(path):
    text = open(path, encoding='utf-8', errors='replace').read()
    found = list(params(text))
    print(f"{path}")
    print(f"  <Preset> {len(text)} bytes, {len(found)} non-empty <Param>")
    if not found:
        # The shape a broken save writes, and worth naming rather than leaving
        # as an empty report: <Preset><Param id="1" val=""/></Preset>.
        print("  NO DOCUMENT -- every parameter is empty")
        return 0
    total_cables = 0
    for pid, raw in found:
        types = module_types(raw)
        cbl = cables(raw)
        total_cables += len(cbl)
        print(f"  Param id={pid}: {len(raw)} bytes, "
              f"{sum(types.values())} modules, {len(types)} types, {len(cbl)} cables")
        if types:
            print("    " + ", ".join(f"{k.decode()}={v}" for k, v in sorted(types.items())))
        for c in cbl:
            print("    " + c.decode('utf-8', 'replace'))
    return total_cables


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    for path in sys.argv[1:]:
        report(path)
    return 0


if __name__ == '__main__':
    sys.exit(main())
