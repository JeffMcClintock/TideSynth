#!/usr/bin/env python3
"""Write a REAPER .rpp that loads TIDE with a given rack already in it.

tests/hosts/README.md said these were GUI artifacts with no script to write
them -- place and cable prefabs by hand in REAPER, then save. That is true of
AUTHORING a rack, and false of getting an ALREADY-BUILT one into a project:
the standalone's session.xml IS the <Preset> element a VST3 instance stores,
so it only has to be re-framed. This does the re-framing.

    python3 scripts/make-host-fixture.py <template.rpp> <preset.xml> <out.rpp>

<preset.xml> is a standalone session.xml, or any file holding a <Preset>
element -- tests/fixtures/e53-vcv-rack-segv.xml is one, and is what E19's
windows VST3 cell was measured with on 2026-08-28.

Written for BACKLOG E19/E59, where the question is what the PROCESSOR does
with a restored rack, and answering it needs a host project carrying one.

The framing was read off the committed tests/hosts/v1-rack.rpp rather than
guessed, and every field this rewrites was identified there:

  <VST> block, line 0   a 44-byte header. int32[0] is the plug-in id -- the
                        same number that leads the .rpp's UID token -- and
                        int32[8] is the STATE BYTE LENGTH. Patch it or REAPER
                        reads a truncated chunk and restores nothing.
  lines 1..n-1          the state: int32 xmlLen+4, int32 1, int32 xmlLen, the
                        XML, then eight zero bytes.
  last line             REAPER's trailer, copied verbatim.

BACKLOG E29 is why the token is rewritten rather than copied: REAPER 7.78 on
Windows and 7.45 on macOS write DIFFERENT byte orders for the SAME UID, and a
fixture carrying the wrong one hangs on a modal "not available" dialog rather
than failing. Output from this script is therefore LOCAL, not committable --
see tests/hosts/README.md.
"""
import base64, re, struct, sys

# BACKLOG E29: REAPER 7.78 on Windows writes a different byte order for the
# same UID than 7.45 on macOS, and the two are mutually exclusive. The
# committed fixtures carry the 7.45 token; this box needs the other one.
TOKEN_WIN = "1558955188{67756C506E694D4750492050A2A07287}"
ID_WIN = 1558955188

XMLDECL = b'<?xml version="1.0" encoding="UTF-8"?>\n'


def build_state(preset_bytes):
    """The VST3 instance state REAPER stores, given a <Preset> element."""
    m = re.search(rb"<Preset.*</Preset>", preset_bytes, re.S)
    if not m:
        raise SystemExit("no <Preset> element in the input")
    xml = XMLDECL + m.group(0) + b"\n"
    return struct.pack("<iii", len(xml) + 4, 1, len(xml)) + xml + b"\x00" * 8


def build_header(state_len):
    h = bytearray(44)
    struct.pack_into("<i", h, 0, ID_WIN)
    struct.pack_into("<i", h, 4, -17998098)
    struct.pack_into("<i", h, 12, 2)
    struct.pack_into("<i", h, 16, 1)
    struct.pack_into("<i", h, 24, 2)
    struct.pack_into("<i", h, 32, state_len)
    struct.pack_into("<i", h, 36, 1)
    struct.pack_into("<i", h, 40, 1114111)
    return bytes(h)


def wrap(b64, width=128):
    return [b64[i:i + width] for i in range(0, len(b64), width)]


def main():
    template, preset_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    text = open(template, encoding="utf-8", errors="replace").read()
    state = build_state(open(preset_path, "rb").read())
    header = build_header(len(state))

    body = [base64.b64encode(header).decode()]
    body += wrap(base64.b64encode(state).decode())
    body.append("AAAQAAAA")          # REAPER's trailer, verbatim from v1-rack.rpp

    lines = text.splitlines()
    out, i = [], 0
    while i < len(lines):
        if lines[i].strip().startswith("<VST "):
            indent = lines[i][: len(lines[i]) - len(lines[i].lstrip())]
            out.append(re.sub(r"\d+\{[0-9A-Fa-f]+\}", TOKEN_WIN, lines[i]))
            out += [indent + "  " + b for b in body]
            depth = 1
            i += 1
            while i < len(lines) and depth:
                s = lines[i].strip()
                if s.startswith("<"):
                    depth += 1
                elif s == ">":
                    depth -= 1
                    if depth == 0:
                        out.append(indent + ">")
                        i += 1
                        break
                i += 1
            continue
        out.append(lines[i])
        i += 1

    open(out_path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print("wrote %s  (state %d bytes, preset from %s)" % (out_path, len(state), preset_path))


if __name__ == "__main__":
    main()
