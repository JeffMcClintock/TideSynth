#!/usr/bin/env python3
"""Frame a <Preset> element into the base64 REAPER stores as a CLAP clap_chunk.

MEASURED, not guessed, exactly as frame_chunk.py's VST3 twin was.
clap-probe.lua read the parm off a DEFAULT TIDE instance in REAPER 7.43/Linux:
116 base64 chars, 86 bytes, and they are

    the preset XML, then ONE trailing zero byte

and nothing else. No length prefix, no parameter index, no trailer -- which is
the whole difference from the VST3 chunk, where the same preset arrives inside
`int32 xmlLen+4 | int32 1 | int32 xmlLen | XML | 8 zero bytes`. A CLAP state
blob IS the plug-in's own bytes; VST3's is SynthEdit's chunk-parameter framing.

Do not "share" the two. They look similar enough to tempt it and the failure is
silent: a CLAP fed the VST3 framing reads a length field as XML and loads the
default, which logs nothing unusual.

Also note the parm name: REAPER answers `clap_chunk` here and `vst_chunk` there,
and asking for the wrong one returns ok=false with an EMPTY string rather than
an error -- so a script that does not check the length will happily mint a
project carrying nothing.

    python3 frame_clap_chunk.py <preset-or-session.xml> <out.b64>
"""
import base64, re, sys


def build_state(preset_bytes):
    m = re.search(rb"<Preset.*</Preset>", preset_bytes, re.S)
    if not m:
        raise SystemExit("no <Preset> element found")
    xmldecl = b'<?xml version="1.0" encoding="UTF-8"?>\n'
    return xmldecl + m.group(0) + b'\n\x00'


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    raw = open(sys.argv[1], 'rb').read()
    state = build_state(raw)
    b64 = base64.b64encode(state).decode()
    open(sys.argv[2], 'w').write(b64)
    print(f"wrote {sys.argv[2]} ({len(b64)} b64 chars, state {len(state)} bytes)")


if __name__ == '__main__':
    main()
