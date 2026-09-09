#!/usr/bin/env python3
# qemu_graphics.py
#
# Shared helpers for scripting a graphical ("-qemu-graphics") QEMU
# session: a QMP client (send-key, screendump) and P6 PPM pixel
# analysis. Extracted from ad hoc techniques used bringing up
# forks/arm-pi1's framebuffer console and USB keyboard - see
# docs/claude_notes/notes_tutorial_qemu.txt for the general technique
# writeup. Used by scripts/test_qemu_graphics.py; not a standalone
# script itself (no __main__ block - import it).
#
# claude: every "run-<arch>-qemu-graphics" Makefile target (top-level
# Makefile, and forks/arm-pi1/forks/arm-pi1-bis's own Makefiles) accepts
# an optional QMP_SOCK variable that adds "-qmp unix:<path>,server,nowait"
# to the QEMU command line - unset by default, so plain interactive use
# is unaffected. That's the socket path this module's QMP class expects.

import json
import socket
import time

# claude: covers what a typical shell command needs (letters, digits,
# space, a handful of punctuation, Enter) - raise KeyError rather than
# silently drop a character for anything not covered, so a test typing
# something unexpected fails loudly instead of sending the wrong thing.
QCODE = {
    " ": "spc",
    "\n": "ret",
    "\r": "ret",
    ".": "dot",
    "/": "slash",
    "-": "minus",
    "_": "minus",  # claude: real shift+- ; simplified, no shift handling below
    "=": "equal",
}
for _ch in "abcdefghijklmnopqrstuvwxyz0123456789":
    QCODE[_ch] = _ch


class QMP:
    """A minimal QEMU Machine Protocol client over a unix socket."""

    def __init__(self, sock_path, connect_timeout=15, recv_timeout=5):
        deadline = time.time() + connect_timeout
        last_err = None
        self.sock = None
        while time.time() < deadline:
            try:
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                s.settimeout(recv_timeout)
                s.connect(sock_path)
                self.sock = s
                break
            except (FileNotFoundError, ConnectionRefusedError) as e:
                last_err = e
                time.sleep(0.2)
        if self.sock is None:
            raise TimeoutError(
                f"could not connect to QMP socket {sock_path!r} "
                f"within {connect_timeout}s: {last_err}"
            )
        self._recv()  # greeting banner
        self.cmd({"execute": "qmp_capabilities"})

    def _recv(self):
        data = self.sock.recv(65536)
        if not data:
            raise ConnectionError("QMP socket closed")
        # claude: a single recv() can return multiple newline-delimited
        # JSON objects (e.g. a stray event before the reply we wanted) -
        # only the LAST one is treated as "the" response, matching how
        # every caller here uses this (fire one command, read one
        # reply); callers that care about intermediate events should
        # read directly instead.
        line = data.decode().strip().splitlines()[-1]
        return json.loads(line)

    def cmd(self, obj, settle=0.15):
        self.sock.sendall((json.dumps(obj) + "\n").encode())
        time.sleep(settle)
        return self._recv()

    def send_key(self, qcode):
        self.cmd({"execute": "send-key", "arguments": {"keys": [{"type": "qcode", "data": qcode}]}})

    def type_text(self, s, delay=0.05):
        """Send a string as a sequence of key presses, per QCODE above."""
        for ch in s:
            if ch not in QCODE:
                raise KeyError(f"no qcode mapping for {ch!r} - extend QCODE in {__name__}")
            self.send_key(QCODE[ch])
            time.sleep(delay)

    def screendump(self, path, settle=0.5):
        self.cmd({"execute": "screendump", "arguments": {"filename": path}})
        time.sleep(settle)  # claude: the file write itself isn't part of the QMP reply

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def load_ppm(path):
    """Parse a raw (P6) PPM file. Returns (width, height, rgb_bytes)."""
    with open(path, "rb") as f:
        data = f.read()
    if not data.startswith(b"P6"):
        raise ValueError(f"{path}: not a raw (P6) PPM file")
    i = 2
    vals = []
    while len(vals) < 3:
        while data[i] in b" \t\r\n":
            i += 1
        if data[i : i + 1] == b"#":
            while data[i] not in b"\n":
                i += 1
            continue
        j = i
        while data[j] not in b" \t\r\n":
            j += 1
        vals.append(int(data[i:j]))
        i = j
    width, height, _maxval = vals
    i += 1  # the single whitespace byte after maxval
    pixels = data[i : i + 3 * width * height]
    return width, height, pixels


def non_black_count(width, height, pixels):
    """Count pixels that aren't pure black (0,0,0) - the cheapest 'is
    anything drawn at all' signal."""
    return sum(1 for k in range(0, 3 * width * height, 3) if pixels[k : k + 3] != b"\x00\x00\x00")


def distinct_colors(width, height, pixels, limit=None):
    """Set of distinct (r,g,b) byte-triples present. 2 (background + one
    ink colour) is the signature of a plain text console; pass `limit`
    to stop early once that many distinct colours are seen (cheaper for
    a "is this clearly more than N colours" check on a large image)."""
    colors = set()
    for k in range(0, 3 * width * height, 3):
        colors.add(pixels[k : k + 3])
        if limit is not None and len(colors) > limit:
            break
    return colors


def ascii_art(width, height, pixels, cell_w=8, cell_h=16, max_rows=30, max_cols=120):
    """A coarse downsample - one character per (cell_w x cell_h) block,
    '#' if any sampled pixel in that block is non-background - cheap
    enough to print directly into a terminal or CI log to eyeball what
    the screen actually shows, without needing to view the PPM itself."""
    lines = []
    for row in range(0, min(height, max_rows * cell_h), cell_h):
        line = []
        for col in range(0, min(width, max_cols * cell_w), cell_w):
            ink = False
            for dy in range(0, cell_h, max(1, cell_h // 4)):
                if ink:
                    break
                for dx in range(0, cell_w, max(1, cell_w // 4)):
                    y, x = row + dy, col + dx
                    if y >= height or x >= width:
                        continue
                    idx = 3 * (y * width + x)
                    if pixels[idx : idx + 3] != b"\x00\x00\x00":
                        ink = True
                        break
            line.append("#" if ink else ".")
        lines.append("".join(line))
    return "\n".join(lines)
