#!/usr/bin/env python3
"""BACKLOG E25 -- which null produced `KERN_INVALID_ADDRESS at 0x50`?

E25 was filed on a macOS crash report whose faulting address is the whole
evidence, because the two candidate nulls on that stack fault at DIFFERENT
addresses and the report names one of them:

    CUG::GetPlug(int)                     <- innermost frame in the report
    CContainer::getIgnoreProgramChange()
    PatchParameter_base::ExportXml(...)
    CPatchManager::ExportXml(...)
    CContainer::ExportXml(...)
    TideApp::exportChunkXml()
    TideApp::importChunkXml(...)

  (A) GetPlug RETURNS null -- the container's plug table is short of
      PN_IGNORE_PC (3), so `GetPlug(3)->GetDefault()` calls a pure virtual
      through nullptr.  That reads the vtable at [this+0], so it faults at
      address 0x0.

  (B) getIgnoreProgramChange is ENTERED with `this == nullptr` -- the caller
      passed a null container.  GetPlug's first instruction reads the Plugs
      vector at [this+0x50], so it faults at address 0x50.

The report says 0x50, so it is (B) and not (A).  That matters because the two
have DIFFERENT fixes in different files, and the fix for (A) cannot repair (B):
with `this` already null the fault happens inside GetPlug, before any guard
added to getIgnoreProgramChange's body could run.

WHY A PROBE AND NOT AN ASSERTION.  Both offsets are properties of the compiled
binary -- the class layout and whether GetPlug got inlined -- not of the source,
so they have to be read off a build rather than reasoned about.  They are also
exactly the kind of fact that rots: add a member to CUG above `Plugs` and 0x50
moves, and this probe says so instead of the next reader quietly trusting a
number written down in 2026.

    python3 tests/e25_null_container_probe.py [--binary PATH] [--run]

Default --binary is the standalone in ./build.  Static mode (the default) needs
only `otool` and answers the (A)-vs-(B) question.  `--run` additionally launches
the app under lldb, forces `this = nullptr` on one call, and reports the signal,
the faulting address and the stack -- the control that shows the offsets above
really do produce the reported crash rather than merely being consistent with it.

Exit status is 0 when the binary still matches the E25 diagnosis, 1 when it does
not (which is a result, not a script failure -- read the output).
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

DEFAULT_BINARY = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "build", "SynthEditSem", "TIDE-Rack.app", "Contents", "MacOS", "TIDE-Rack",
)

GETPLUG = "__ZN3CUG7GetPlugEi"
GETIGNORE = "__ZN10CContainer22getIgnoreProgramChangeEv"
IGNOREPC = "__ZN19PatchParameter_base19ignoreProgramChangeEv"

# The address the crash report names.  Not a constant of the source -- see the
# module docstring; the probe's job is to check the binary still produces it.
REPORTED_FAULT_ADDRESS = 0x50


def disassemble(binary, symbol):
    """Return the disassembly lines of one symbol, or None if it is absent."""
    try:
        out = subprocess.run(
            ["otool", "-tV", "-p", symbol, binary],
            capture_output=True, text=True, check=False,
        ).stdout
    except FileNotFoundError:
        sys.exit("otool not found -- this probe is macOS-only")

    if "Can't find" in out or symbol not in out:
        return None

    lines = []
    started = False
    for line in out.splitlines():
        if line.startswith(symbol + ":"):
            started = True
            continue
        if not started:
            continue
        # the next symbol label ends this one
        if re.match(r"^_?_\w+:", line):
            break
        if re.match(r"^[0-9a-f]{8,16}\s", line):
            lines.append(line)
    return lines


def first_this_offset(lines):
    """Offset off x0 that the function reads BEFORE it can branch away.

    GetPlug's prologue is `tbnz w1, #0x1f, ...` (the negative-index guard, which
    tests the ARGUMENT not `this`) followed by the load off x0.  Anything that
    dereferences x0 before a `cbz x0` is what a null `this` faults on.
    """
    for line in lines:
        body = line.split("\t", 1)[1] if "\t" in line else line
        # a null-check on x0 means the function survives a null this
        if re.search(r"\bcbz\s+x0\b", body):
            return None
        m = re.search(r"\b(ldr|ldp|ldrb|ldrsb|ldrh)\b.*\[x0(?:,\s*#(0x[0-9a-f]+|\d+))?\]", body)
        if m:
            return int(m.group(2), 0) if m.group(2) else 0
    return None


def static_check(binary):
    print(f"binary: {binary}")
    if not os.path.exists(binary):
        sys.exit(f"no such binary: {binary}\n(build it, or pass --binary)")

    ok = True

    gp = disassemble(binary, GETPLUG)
    if gp is None:
        print(f"  {GETPLUG}: ABSENT (inlined?) -- cannot confirm the offset")
        return False
    off = first_this_offset(gp)
    print(f"  CUG::GetPlug         first read off `this`: "
          f"{'none (null-safe)' if off is None else hex(off)}")
    print(f"    {gp[0].split(chr(9), 1)[-1] if gp else ''}")
    print(f"    {gp[1].split(chr(9), 1)[-1] if len(gp) > 1 else ''}")
    if off != REPORTED_FAULT_ADDRESS:
        print(f"    MISMATCH: E25's report faults at {hex(REPORTED_FAULT_ADDRESS)}; "
              f"this build would fault at {hex(off) if off is not None else 'no fault'}")
        ok = False
    else:
        print(f"    -> a null `this` here faults at {hex(off)} = the reported address")

    gi = disassemble(binary, GETIGNORE)
    if gi is None:
        print(f"  {GETIGNORE}: ABSENT")
        return False
    # candidate (A): the returned IPlug* is dereferenced for a virtual call
    calls_getplug = any("GetPlugEi" in l for l in gi)
    deref_after = None
    for i, l in enumerate(gi):
        if "GetPlugEi" in l:
            for nxt in gi[i + 1:i + 3]:
                m = re.search(r"\bldr\b\s+x\d+,\s*\[x0\]", nxt)
                if m:
                    deref_after = 0
            break
    print(f"  CContainer::getIgnoreProgramChange: calls GetPlug={calls_getplug}, "
          f"null-return would fault at "
          f"{hex(deref_after) if deref_after is not None else 'unknown'}")
    if deref_after == 0:
        print("    -> candidate (A) (short plug table) would fault at 0x0, NOT 0x50")

    ipc = disassemble(binary, IGNOREPC)
    if ipc is None:
        print(f"  PatchParameter_base::ignoreProgramChange: ABSENT "
              f"(inlined into ExportXml -- expected at -O2)")
    else:
        guarded_module = any(re.search(r"\bcbz\s+x\d+", l) for l in ipc)
        tail = any(re.search(r"^\s*\w+\s+b\s+" + GETIGNORE, l) or
                   (l.split("\t")[-1].startswith("b\t") and GETIGNORE in l) for l in ipc)
        print(f"  PatchParameter_base::ignoreProgramChange: "
              f"module() null-checked={guarded_module}, tail-calls getIgnoreProgramChange={tail}")
        for l in ipc:
            body = l.split("\t", 1)[1] if "\t" in l else l
            print(f"    {body}")
        print("    -> the load between the cbz and the branch is module()->Container(),")
        print("       and nothing checks it.  That is the null that reaches 0x50.")

    return ok


# `breakpoint set -n` wants the symbol WITHOUT the Mach-O leading underscore that
# `otool -p` requires.  Passing otool's spelling sets a breakpoint that never
# resolves, and lldb reports that as silence rather than as an error.
LLDB_SCRIPT = """\
breakpoint set -n {sym}
breakpoint command add -s python 1
import lldb
global _hits
try: _hits
except: _hits = 0
_hits += 1
if _hits == 2:
    print("E25-PROBE: forcing this=NULL on call 2 (module()->Container() == nullptr)")
    frame.reg['x0'].SetValueFromCString('0')
return False
DONE
run
"""


def run_check(binary):
    if shutil.which("lldb") is None:
        sys.exit("lldb not found -- --run needs it")

    tmp = tempfile.mkdtemp(prefix="e25probe.")
    script = os.path.join(tmp, "e25.lldb")
    with open(script, "w") as f:
        # exactly ONE underscore comes off -- lstrip("_") would eat both and
        # leave a name that resolves to nothing
        f.write(LLDB_SCRIPT.format(sym=GETIGNORE[1:]))

    env = dict(os.environ, HOME=tmp)          # isolated HOME: never touch a real session
    print(f"\nlaunching under lldb with HOME={tmp} ...")
    try:
        out = subprocess.run(
            ["lldb", "-b", "-s", script,
             "-o", "register read x0", "-o", "bt 8", "-o", "kill", "--", binary],
            capture_output=True, text=True, timeout=180, env=env,
        ).stdout
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or b"").decode("utf8", "replace")

    interesting = [l for l in out.splitlines()
                   if "E25-PROBE" in l or "stop reason" in l or "frame #" in l
                   or "Breakpoint 1:" in l]
    for l in interesting:
        print("  " + l.strip())

    if "E25-PROBE" not in out:
        print("  the breakpoint never hit twice -- getIgnoreProgramChange is called "
              "3x at startup on a stock build, so this means it did not resolve "
              "(check the symbol spelling) or the app died first")
        return False

    m = re.search(r"EXC_BAD_ACCESS \(code=\d+, address=(0x[0-9a-f]+)\)", out)
    if not m:
        print("  no EXC_BAD_ACCESS seen -- the forced null did not fault "
              "(did the breakpoint hit twice?)")
        return False
    got = int(m.group(1), 0)
    print(f"\n  faulted at {hex(got)}; E25's crash report says "
          f"{hex(REPORTED_FAULT_ADDRESS)}")
    return got == REPORTED_FAULT_ADDRESS


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--binary", default=DEFAULT_BINARY)
    ap.add_argument("--run", action="store_true",
                    help="also launch under lldb and force the null (the control)")
    args = ap.parse_args()

    ok = static_check(args.binary)
    if args.run:
        ok = run_check(args.binary) and ok

    print("\nRESULT:", "binary still matches E25's diagnosis" if ok
          else "binary does NOT match -- re-read the offsets above before trusting the row")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
