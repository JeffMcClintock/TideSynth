#!/usr/bin/env python3
"""Decode the TIDE VST state out of a REAPER .rpp and report what the chunk holds.

Usage: decode_rpp.py [--preset-out <file>] <project.rpp>

--preset-out writes the OUTER <Preset> element -- the GMPI preset XML, the thing
`gmpiController.getPreset()` returns and `setPresetXmlFromDaw()` accepts -- to a
file, rather than only reporting on it. That is what a non-VST3 host needs to
restore the same rack: the AU3 wrapper carries it verbatim as the GMPIPRESET key
of `fullState` (GMPI_Wrappers/wrapper/AU3/AU3_Wrapper.mm:509,522), so a fixture
saved from REAPER's VST3 can drive the AU without being re-authored by hand.
Used by tests/e9_au_rate_probe.mm (BACKLOG E9).

The inner <Document> is what the existing report already writes per-Param; the
two are different things and it is worth keeping them straight -- the document
alone is NOT restorable, because the wrapper looks for the <Preset> wrapper.
"""
import base64
import re
import sys


def vst_blocks(text):
    """Yield the base64 payload of each <VST ...> block."""
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        if lines[i].strip().startswith('<VST '):
            depth = 1
            payload = []
            i += 1
            while i < len(lines) and depth:
                s = lines[i].strip()
                if s.startswith('<'):
                    depth += 1
                elif s == '>':
                    depth -= 1
                    if depth == 0:
                        break
                else:
                    payload.append(s)
                i += 1
            yield '\n'.join(payload)
        i += 1


def main():
    args = sys.argv[1:]
    preset_out = None
    if args and args[0] == '--preset-out':
        if len(args) < 3:
            print('usage: decode_rpp.py [--preset-out <file>] <project.rpp>')
            return 2
        preset_out = args[1]
        args = args[2:]
    if len(args) != 1:
        print('usage: decode_rpp.py [--preset-out <file>] <project.rpp>')
        return 2

    path = args[0]
    text = open(path, 'r', errors='replace').read()

    found_any = False
    for bi, payload in enumerate(vst_blocks(text)):
        lines = payload.splitlines()
        # REAPER wraps the state's base64 across many lines, with a header line
        # before it and a trailer after. Try every contiguous run and keep the
        # longest decode that actually contains a <Preset>.
        candidates = []
        for start in range(len(lines)):
            for end in range(start + 1, len(lines) + 1):
                candidates.append((f'{start}..{end - 1}', ''.join(lines[start:end])))
        candidates.sort(key=lambda c: -len(c[1]))
        seen_best = False
        for li, line in candidates:
            if seen_best:
                break
            try:
                raw = base64.b64decode(line, validate=False)
            except Exception:
                continue
            if b'<Preset' not in raw:
                continue

            found_any = True
            seen_best = True
            print(f'=== VST block {bi}, base64 lines {li}: {len(raw)} bytes decoded ===')
            m = re.search(rb'<Preset.*?</Preset>', raw, re.S)
            preset = m.group(0) if m else raw
            print(f'    outer preset: {len(preset)} bytes')

            if preset_out:
                # Only the first block's preset is written. A .rpp with two TIDE
                # instances would need the caller to say which; none of the
                # fixtures has one, and guessing silently is worse than the
                # limitation.
                open(preset_out, 'wb').write(preset)
                print(f'    preset written: {preset_out}')
                preset_out = None

            for pm in re.finditer(rb'<Param\s+id="(\d+)"\s+val="([^"]*)"', preset):
                pid = pm.group(1).decode()
                val = pm.group(2)
                print(f'    Param id={pid}: val is {len(val)} chars')
                if len(val) < 32:
                    print(f'        -> {val.decode(errors="replace")!r}')
                    continue
                try:
                    inner = base64.b64decode(val, validate=False)
                except Exception as e:
                    print(f'        -> not base64 ({e})')
                    continue
                print(f'        -> decodes to {len(inner)} bytes')
                for tag in (b'<Document', b'<DSP', b'<Editor', b'<master_container'):
                    n = inner.count(tag)
                    mark = 'yes' if n else 'NO '
                    print(f'           {mark} {tag.decode():<18} x{n}')
                mods = re.findall(rb'<Module\s+[^>]*Type="([^"]+)"', inner)
                if mods:
                    print(f'           modules: {[m.decode() for m in mods]}')
                out = path + f'.block{bi}.param{pid}.xml'
                open(out, 'wb').write(inner)
                print(f'           written: {out}')

    if not found_any:
        print('no <Preset> found in any VST block')


if __name__ == '__main__':
    sys.exit(main() or 0)
