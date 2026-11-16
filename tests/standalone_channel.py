#!/usr/bin/env python3
"""The standalone app's command channel, on every platform it has one.

BACKLOG E19 (win/STANDALONE). Five probes drive the GMPI standalone over its
command channel -- e38, e51, e62, e65, e82 -- and until this module every one
of them opened `socket.AF_UNIX` directly, so **none of them could run on
Windows at all**, where the server publishes a NAMED PIPE instead. The
Windows box therefore had no standalone venue, and E19's `win: STANDALONE`
clause had never been measured by anybody. Found and recorded by the
2026-11-10 windows run; this is the port it said was owed.

THE TWO TRANSPORTS, and the one protocol difference that matters:

  posix    `$GMPI_STANDALONE_IPC_DIR/gmpi-standalone.<pid>`, an AF_UNIX
           SOCK_STREAM socket file. The client may half-close its write end,
           so a request is framed by `shutdown(SHUT_WR)` and the reply by EOF.

  windows  `\\\\.\\pipe\\gmpi-standalone.<pid>`, byte-mode and duplex
           (`IpcServerWin.h`, `kPipePrefix`). **A named pipe has no
           half-close**, so the EOF framing above does not exist here: closing
           the write end closes the whole connection and the replies with it.

What replaces it is exact rather than a heuristic. `CommandProtocol::respondTo`
runs ONE command line and returns ONE response line -- the handler's answer
with a '\\n' appended, unconditionally -- and `LineFramer` calls it once per
line received. So the reply to N command lines is exactly N newline-terminated
lines, and the Windows client reads until it has counted N of them. No idle
timer, no sentinel, and no guess about how long the app may take: a verb that
blocks for twenty seconds is waited for, and a wedged app still fails on the
deadline rather than hanging.

THE PIPE NAME IS CONSTRUCTED, NOT SEARCHED, and that is deliberate. The unix
server's socket lives inside a private `GMPI_STANDALONE_IPC_DIR` the probe
makes itself, so a glob there can only find this probe's app. **The Windows
pipe namespace is global**, so the same glob would also match a standalone the
developer happens to have open -- and a probe that drove the wrong app would
measure the wrong thing while passing. The server names its pipe with
`GetCurrentProcessId()`, and we know the pid we launched, so the address is
derived from that pid alone. It cannot resolve to anybody else's app.

`GMPI_STANDALONE_IPC_DIR` is still set on Windows, and ignored there by design:
one env block for both platforms is cheaper than a conditional at every call
site, and a future transport that wants a directory will find it already there.
"""

import glob
import json
import os
import pathlib
import socket
import sys
import tempfile
import threading
import time

WINDOWS = sys.platform == "win32"

if WINDOWS:
    import ctypes
    from ctypes import wintypes

    _K32 = ctypes.WinDLL("kernel32", use_last_error=True)
    _K32.WaitNamedPipeW.restype = wintypes.BOOL
    _K32.WaitNamedPipeW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD]

    ERROR_FILE_NOT_FOUND = 2
    ERROR_SEM_TIMEOUT = 121
    ERROR_PIPE_BUSY = 231

#: AF_UNIX `sun_path` is 104 bytes on macOS and 108 on Linux; 103 usable chars
#: is the portable floor. The leaf below is `gmpi-standalone.` plus a pid.
SUN_PATH_MAX = 103
LEAF_PREFIX = "gmpi-standalone."


class ChannelError(RuntimeError):
    """The channel could not be reached, or did not answer in time."""


def tempdir(prefix):
    """A scratch directory the standalone may use, on either platform.

    The existing probes pass `prefix="/tmp/e82-"` to `mkdtemp`, which on
    Windows names a directory *inside the current drive's root* called `tmp`
    -- it is not an error and not `%TEMP%`, so it half-works and litters.
    Splitting the leaf from the directory is what makes one call correct in
    both places.
    """
    return pathlib.Path(tempfile.mkdtemp(prefix=pathlib.PurePath(prefix).name))


def launch_env(ipc_dir, config_dir, base=None):
    """The environment the standalone is launched with."""
    return dict(
        base if base is not None else os.environ,
        GMPI_STANDALONE_IPC_DIR=str(ipc_dir),
        GMPI_STANDALONE_CONFIG_DIR=str(config_dir),
    )


def path_is_usable(ipc_dir):
    """`(ok, detail)` -- whether a channel address will fit in this directory.

    Only the unix transport has a length limit, and exceeding it is a setup
    failure a probe should report rather than a bind error to puzzle over.
    """
    if WINDOWS:
        return True, "named pipe -- the ipc directory is not part of the address"
    projected = len(str(ipc_dir)) + 1 + len(LEAF_PREFIX) + 7
    if projected > SUN_PATH_MAX:
        return False, f"projected sun_path {projected} > {SUN_PATH_MAX}: {ipc_dir}"
    return True, f"projected sun_path {projected} of {SUN_PATH_MAX}"


def pipe_published(name):
    """Does this named pipe exist, without connecting to it?

    **The obvious test -- open it and close it again -- is wrong here, and
    cheaply so.** Opening CONSUMES a pipe instance: the server hands the
    connection to a client thread, which reads zero bytes and disconnects. The
    real server re-arms before it serves, so it survives that; a server holding
    its last instance does not, and the caller has spent the connection it was
    about to need. Measured while writing this: the self-test's stand-in server
    was reduced to a closed pipe by its own discovery call, and the failure
    surfaced one call later as a `FileNotFoundError` from `talk()`.

    `WaitNamedPipeW` asks the same question without connecting, and its errors
    separate the two cases a probe must not confuse:

      True                   published, with an instance free
      ERROR_FILE_NOT_FOUND   not published -- the app has not got there yet
      ERROR_SEM_TIMEOUT      published, every instance busy
      ERROR_PIPE_BUSY        published, every instance busy

    Both "busy" answers count as published, because the question is whether the
    app has a channel, not whether this instant is a good time to use it.
    """
    ctypes.set_last_error(0)
    if _K32.WaitNamedPipeW(name, 0):
        return True
    return ctypes.get_last_error() in (ERROR_SEM_TIMEOUT, ERROR_PIPE_BUSY)


def wait_for_channel(ipc_dir, pid, timeout=60.0, poll=0.5, is_alive=None):
    """Block until the app published its channel, and return the address.

    `is_alive` is an optional callable: when it answers False the wait ends at
    once, so an app that died on startup is reported as such instead of being
    waited out for the whole timeout.
    """
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if WINDOWS:
            name = r"\\.\pipe\%s%d" % (LEAF_PREFIX, pid)
            if pipe_published(name):
                return name
        else:
            hits = glob.glob(str(pathlib.Path(ipc_dir) / (LEAF_PREFIX + "*")))
            if hits:
                return hits[0]
        if is_alive is not None and not is_alive():
            raise ChannelError("the app exited before publishing a command channel")
        time.sleep(poll)
    raise ChannelError(f"no command channel after {timeout:g}s (pid {pid})")


def _parse(blob):
    out = []
    for line in blob.decode("utf-8", "replace").splitlines():
        line = line.strip()
        if line.startswith("{"):
            try:
                out.append(json.loads(line))
            except json.JSONDecodeError:
                pass
    return out


def _talk_unix(address, payload, timeout):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect(address)
    s.sendall(payload)
    s.shutdown(socket.SHUT_WR)
    buf = b""
    try:
        while True:
            chunk = s.recv(65536)
            if not chunk:
                break
            buf += chunk
    except socket.timeout:
        pass
    s.close()
    return buf


def _talk_pipe(address, payload, expected, timeout):
    """Write the request, then read exactly `expected` response lines.

    The read runs on a helper thread so the deadline is real. A thread still
    blocked in `ReadFile` when the deadline passes is abandoned rather than
    cancelled -- it is a daemon, and a probe whose channel stopped answering
    is failing anyway. `CancelSynchronousIo` would be the tidy ending and
    needs the thread's own handle; it would not change any outcome a probe
    can observe, so it is left undone on purpose rather than forgotten.
    """
    try:
        pipe = open(address, "r+b", buffering=0)
    except OSError as exc:
        raise ChannelError(f"cannot open {address}: {exc}") from exc

    chunks = []
    done = threading.Event()

    def reader():
        try:
            while not done.is_set():
                chunk = pipe.read(65536)
                if not chunk:
                    break
                chunks.append(chunk)
                if b"".join(chunks).count(b"\n") >= expected:
                    break
        except OSError:
            pass
        finally:
            done.set()

    try:
        pipe.write(payload)
        pipe.flush()
    except OSError as exc:
        pipe.close()
        raise ChannelError(f"cannot write to {address}: {exc}") from exc

    t = threading.Thread(target=reader, daemon=True)
    t.start()
    t.join(timeout)
    timed_out = not done.is_set()
    done.set()

    buf = b"".join(chunks)
    if timed_out:
        # Leave the handle to the abandoned thread; closing it under an
        # in-flight ReadFile is the one thing guaranteed to be worse.
        raise ChannelError(
            f"{address}: {buf.count(chr(10).encode())} of {expected} response "
            f"line(s) in {timeout:g}s"
        )
    pipe.close()
    return buf


def talk(address, verbs, timeout=30.0):
    """Send `verbs` (one command per element) and return the parsed replies."""
    verbs = list(verbs)
    if not verbs:
        return []
    payload = "".join(v + "\n" for v in verbs).encode()
    if WINDOWS:
        buf = _talk_pipe(address, payload, len(verbs), timeout)
    else:
        buf = _talk_unix(address, payload, timeout)
    return _parse(buf)
