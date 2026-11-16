#!/usr/bin/env python3
"""A positive control for `standalone_channel`, with no app and no plugin.

BACKLOG E19 (win/STANDALONE). The transport this exercises is the one thing
the standalone probes cannot check for themselves: if `talk()` is broken they
fail in the plug-in's language -- a missing menu, a window of the wrong size --
and the real fault is three layers below. So the transport gets its own test,
against a stand-in server that imitates the one protocol rule the real one
guarantees (`CommandProtocol::respondTo`: one response line per command line)
and nothing else.

WHAT THIS CAN AND CANNOT SHOW. It proves the client finds the published
address, writes a multi-verb request, reads every reply back and parses them in
order -- and, on Windows, that it stops reading at the right count rather than
blocking forever, which is the half-close the pipe does not have. It proves
NOTHING about TIDE, the rack, or any verb's meaning; the stand-in answers
`{"cmd": ..., "ok": true}` to anything. The arms that must FAIL are the point
of the file: a count that never arrives has to time out rather than hang, and a
missing channel has to raise rather than wait out the clock.

Run: `python3 tests/standalone_channel_selftest.py`. Exit 0 pass, 1 fail.
"""

import json
import os
import pathlib
import socket
import sys
import threading
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import standalone_channel as chan

FAKE_PID = os.getpid()


class StandIn:
    """One response line per command line, then wait for more. No half-close."""

    def __init__(self, ipc_dir, pid, mute_after=None):
        #: `mute_after` stops answering after N commands, to drive the timeout
        #: arm. The real server has no such mode; a wedged main thread is what
        #: it would look like.
        self.mute_after = mute_after
        self.served = 0
        self.stop = threading.Event()
        self.thread = None
        if chan.WINDOWS:
            self.address = r"\\.\pipe\%s%d" % (chan.LEAF_PREFIX, pid)
        else:
            self.address = str(pathlib.Path(ipc_dir) / (chan.LEAF_PREFIX + str(pid)))

    def _reply(self, line):
        self.served += 1
        if self.mute_after is not None and self.served > self.mute_after:
            return None
        return json.dumps({"cmd": line.decode().strip().lstrip("-").split(" ")[0],
                           "ok": True, "n": self.served}).encode() + b"\n"

    # -- windows ------------------------------------------------------------
    def _serve_pipe(self):
        """One connection after another, like the real server.

        It re-arms rather than serving once because the real one does -- and
        because a single-shot server made the client's old open()-to-test-for-
        existence look correct on the first call and broken on the second.
        """
        import ctypes
        from ctypes import wintypes

        k32 = ctypes.WinDLL("kernel32", use_last_error=True)
        k32.CreateNamedPipeW.restype = wintypes.HANDLE
        INVALID = ctypes.c_void_p(-1).value
        buf = ctypes.create_string_buffer(4096)

        while not self.stop.is_set():
            handle = k32.CreateNamedPipeW(
                self.address,
                0x00000003,           # PIPE_ACCESS_DUPLEX
                0x00000000,           # PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
                4, 4096, 4096, 0, None)
            if not handle or handle == INVALID:
                raise OSError(ctypes.get_last_error(), "CreateNamedPipeW failed")
            k32.ConnectNamedPipe(handle, None)
            pending = b""
            while not self.stop.is_set():
                got = wintypes.DWORD(0)
                if not k32.ReadFile(handle, buf, 4096, ctypes.byref(got), None) or got.value == 0:
                    break
                pending += buf.raw[:got.value]
                while b"\n" in pending:
                    line, pending = pending.split(b"\n", 1)
                    reply = self._reply(line)
                    if reply is None:
                        continue
                    written = wintypes.DWORD(0)
                    k32.WriteFile(handle, reply, len(reply), ctypes.byref(written), None)
            k32.FlushFileBuffers(handle)
            k32.DisconnectNamedPipe(handle)
            k32.CloseHandle(handle)

    # -- posix --------------------------------------------------------------
    def _serve_unix(self):
        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        srv.bind(self.address)
        srv.listen(4)
        srv.settimeout(0.5)
        while not self.stop.is_set():
            try:
                conn, _ = srv.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            pending = b""
            conn.settimeout(10)
            while not self.stop.is_set():
                try:
                    chunk = conn.recv(4096)
                except socket.timeout:
                    break
                if not chunk:
                    break
                pending += chunk
                while b"\n" in pending:
                    line, pending = pending.split(b"\n", 1)
                    reply = self._reply(line)
                    if reply is None:
                        continue
                    conn.sendall(reply)
            conn.close()
        srv.close()
        try:
            os.unlink(self.address)
        except OSError:
            pass

    def __enter__(self):
        target = self._serve_pipe if chan.WINDOWS else self._serve_unix
        self.thread = threading.Thread(target=target, daemon=True)
        self.thread.start()
        time.sleep(0.4)        # let the server reach its accept/connect
        return self

    def __exit__(self, *exc):
        # A server parked in ConnectNamedPipe never re-reads a flag, so the
        # flag alone does not retire it. Connecting once is what wakes it --
        # and leaving it running is not harmless: a second stand-in at the
        # SAME address would race it for connections, and the client would
        # sometimes be answered by the one that is shutting down. (Which is
        # exactly how this was found: an EINVAL on write, two arms later.)
        self.stop.set()
        if chan.WINDOWS:
            try:
                with open(self.address, "r+b", buffering=0):
                    pass
            except OSError:
                pass
        if self.thread is not None:
            self.thread.join(5)
        return False


def main():
    failures = []

    def check(ok, name, detail):
        print(f"  {'PASS' if ok else 'FAIL'}  {name}\n        {detail}")
        if not ok:
            failures.append(name)

    print(f"standalone_channel self-test -- transport={'named pipe' if chan.WINDOWS else 'AF_UNIX'}\n")

    ipc = chan.tempdir("chanselftest-")
    ok, detail = chan.path_is_usable(ipc)
    check(ok, "0a the ipc directory can carry a channel address", detail)

    # 1. a channel that is not there must RAISE, promptly -- the control that
    #    makes every pass below mean something.
    t0 = time.monotonic()
    try:
        chan.wait_for_channel(ipc, FAKE_PID + 31337, timeout=1.5, poll=0.25)
        check(False, "1a MUST-FAIL control: absent channel raises", "it returned an address")
    except chan.ChannelError as exc:
        waited = time.monotonic() - t0
        check(1.0 <= waited < 6.0, "1a MUST-FAIL control: absent channel raises, on the deadline",
              f"{exc} after {waited:.2f}s")

    # 2. is_alive short-circuits the wait: a dead app is not waited out.
    t0 = time.monotonic()
    try:
        chan.wait_for_channel(ipc, FAKE_PID + 31338, timeout=30, poll=0.25,
                              is_alive=lambda: False)
        check(False, "1b MUST-FAIL control: a dead app ends the wait", "it returned an address")
    except chan.ChannelError as exc:
        waited = time.monotonic() - t0
        check(waited < 5.0, "1b MUST-FAIL control: a dead app ends the wait at once, not on the timeout",
              f"{exc} after {waited:.2f}s")

    # 3. the round trip
    with StandIn(ipc, FAKE_PID + 1) as srv:
        addr = chan.wait_for_channel(ipc, FAKE_PID + 1, timeout=10, poll=0.2)
        check(addr == srv.address, "2a the published channel is found at the pid-derived address",
              f"{addr}")
        replies = chan.talk(addr, ["--info"], timeout=10)
        check(len(replies) == 1 and replies[0].get("ok") is True,
              "2b one verb -> exactly one parsed reply", json.dumps(replies))

    with StandIn(ipc, FAKE_PID + 2) as srv:
        addr = chan.wait_for_channel(ipc, FAKE_PID + 2, timeout=10, poll=0.2)
        verbs = ["--pointer-down 1,2", "--pointer-up 1,2", "--context-menu 3,4", "--info"]
        replies = chan.talk(addr, verbs, timeout=10)
        check(len(replies) == len(verbs),
              "2c four verbs -> four replies, and the reader stopped at the fourth",
              f"{len(replies)} replies: {[r.get('cmd') for r in replies]}")
        check([r.get("n") for r in replies] == [1, 2, 3, 4],
              "2d the replies come back IN ORDER", f"{[r.get('n') for r in replies]}")

    # 4. MUST-FAIL: a server that stops answering must time out, not hang.
    #    This is the arm the named pipe needs and the socket got for free.
    with StandIn(ipc, FAKE_PID + 3, mute_after=1) as srv:
        addr = chan.wait_for_channel(ipc, FAKE_PID + 3, timeout=10, poll=0.2)
        t0 = time.monotonic()
        try:
            chan.talk(addr, ["--info", "--info"], timeout=2.0)
            ok, detail = (not chan.WINDOWS), "returned (posix: EOF on half-close ends the read)"
        except chan.ChannelError as exc:
            ok, detail = True, str(exc)
        waited = time.monotonic() - t0
        check(ok and waited < 8.0,
              "3a MUST-FAIL control: a server that answers once and goes quiet does not hang",
              f"{detail} after {waited:.2f}s")

    print()
    if failures:
        print(f"FAILED -- {len(failures)}: {', '.join(failures)}")
        return 1
    print("OK -- the command-channel client finds, writes, reads and frames correctly, "
          "and fails rather than hangs when it cannot.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
