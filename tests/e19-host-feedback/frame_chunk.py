#!/usr/bin/env python3
"""Frame a <Preset> element into the base64 REAPER stores as a VST3 vst_chunk.

MEASURED, not guessed. prepare.lua's dump pass read the parm back off a DEFAULT
TIDE instance in REAPER 7.43/Linux -- 140 base64 chars, 105 bytes -- and it is
exactly:

    int32 xmlLen+4 (89) | int32 1 | int32 xmlLen (85) | the XML | 8 zero bytes

i.e. `build_state` from scripts/make-host-fixture.py and NOTHING else. The
44-byte header and the "AAAQAAAA" trailer that script also writes belong to the
<VST> block in a .rpp file, not to this parm.

WHY THIS EXISTS BESIDE make-host-fixture.py. That script rewrites a .rpp on
disk, so it must know the plug-in TOKEN, which BACKLOG E29 records as
byte-order-divergent per platform -- it carries a hard-coded Windows constant.
Handing the same bytes to TrackFX_SetNamedConfigParm instead lets REAPER mint
its own token when it instantiates the plug-in by name, so the platform
question never arises and no fixture can carry the wrong one.

    python3 frame_chunk.py <preset-or-session.xml> <out.b64>
"""
import base64, re, struct, sys

XMLDECL = b'<?xml version="1.0" encoding="UTF-8"?>\n'


def build_state(preset_bytes):
    m = re.search(rb"<Preset.*</Preset>", preset_bytes, re.S)
    if not m:
        raise SystemExit("no <Preset> element in the input")
    xml = XMLDECL + m.group(0) + b"\n"
    return struct.pack("<iii", len(xml) + 4, 1, len(xml)) + xml + b"\x00" * 8


def main():
    src, out = sys.argv[1], sys.argv[2]
    state = build_state(open(src, "rb").read())
    b64 = base64.b64encode(state).decode()
    open(out, "w").write(b64)
    print("wrote %s (%d b64 chars, state %d bytes)" % (out, len(b64), len(state)))


if __name__ == "__main__":
    main()
