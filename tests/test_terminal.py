#!/usr/bin/env python3
"""Exercise simple-md against a real pseudo-terminal."""

import errno
import fcntl
import os
import pathlib
import pty
import re
import select
import struct
import subprocess
import sys
import termios
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
BINARY = ROOT / "simple-md"
FIXTURE = ROOT / "tests" / "fixtures" / "sample.md"
LONG_FIXTURE = ROOT / "tests" / "fixtures" / "long.md"


def run_viewer(
    quit_key=None,
    use_stdin: bool = False,
    fixture: pathlib.Path = FIXTURE,
    extra_args=(),
    expect_pager: bool = True,
) -> bytes:
    master, slave = pty.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 72, 0, 0))

    def configure_child() -> None:
        os.setsid()
        fcntl.ioctl(slave, termios.TIOCSCTTY, 0)

    command = [str(BINARY), "--width=72", *extra_args]
    if not use_stdin:
        command.append(str(fixture))

    process = subprocess.Popen(
        command,
        stdin=subprocess.PIPE if use_stdin else slave,
        stdout=slave,
        stderr=slave,
        cwd=ROOT,
        close_fds=True,
        preexec_fn=configure_child,
        env={**os.environ, "TERM": "xterm-256color"},
    )
    os.close(slave)
    if use_stdin:
        assert process.stdin is not None
        process.stdin.write(fixture.read_bytes())
        process.stdin.close()

    output = bytearray()
    key_sent = False
    deadline = time.monotonic() + 5
    try:
        while time.monotonic() < deadline:
            readable, _, _ = select.select([master], [], [], 0.05)
            if readable:
                try:
                    chunk = os.read(master, 65536)
                except OSError as error:
                    if error.errno == errno.EIO:
                        break
                    raise
                if not chunk:
                    break
                output.extend(chunk)

            if quit_key is not None and not key_sent and b"\x1b[?1049h" in output:
                os.write(master, quit_key)
                key_sent = True

            # Keep draining the PTY after process exit; terminal output can
            # remain queued after the child has already changed state.
            if process.poll() is not None and not readable:
                break
        else:
            process.kill()
            raise AssertionError("viewer did not exit within five seconds")
    finally:
        os.close(master)

    return_code = process.wait(timeout=1)
    plain_output = re.sub(rb"\x1b\[[0-?]*[ -/]*[@-~]", b"", output)
    assert return_code == 0, f"viewer exited with status {return_code}"
    assert b"Simple Markdown" in plain_output, "rendered heading was not observed"
    if fixture == FIXTURE:
        assert "┌".encode() in output, "rendered table border was not observed"
    if expect_pager:
        assert key_sent, "viewer never accepted the requested quit key"
        assert b"Line 1/" in plain_output, "viewport status was not observed"
        assert output.count(b"\x1b[?1049h") == 1, (
            "alternate screen must be entered exactly once"
        )
        assert output.count(b"\x1b[?1049l") == 1, (
            "alternate screen must be restored exactly once"
        )
        assert output.count(b"\x1b[?25l") == 1, "cursor must be hidden exactly once"
        assert output.count(b"\x1b[?25h") == 1, "cursor must be restored exactly once"
    else:
        assert b"\x1b[?1049h" not in output, "output unexpectedly opened pager"
        assert b"\x1b[?1049l" not in output, "output unexpectedly closed pager"
    return bytes(output)


def main() -> int:
    run_viewer(b"q", extra_args=("--pager=always",))
    run_viewer(b"\x1b", use_stdin=True, extra_args=("--pager=always",))
    run_viewer(expect_pager=False)
    run_viewer(b"q", fixture=LONG_FIXTURE)
    run_viewer(fixture=LONG_FIXTURE, extra_args=("--pager=never",), expect_pager=False)
    print("test_terminal: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
