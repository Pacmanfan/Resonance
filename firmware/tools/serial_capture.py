#!/usr/bin/env python3
"""Capture the device's serial output for a while, optionally sending commands.

    serial_capture.py [--port /dev/ttyUSB0] [--seconds 8] [--no-reset] [--send "report" ...]

Opening the port restarts the board (the USB adapter's control lines drive its
auto-reset circuit), so a capture normally starts at boot. --no-reset opens
the port without touching the control lines' state afterwards and usually
still restarts the board; there is no reliable way to attach without a reset
on this adapter. Each --send line is written at --at seconds from the start
(default 6, after the boot report), then one every 4 seconds.
Needs pyserial (the PlatformIO environment has it).
"""
import argparse
import sys
import time

import serial


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--seconds", type=float, default=8.0)
    ap.add_argument("--no-reset", action="store_true")
    ap.add_argument("--send", action="append", default=[])
    ap.add_argument("--at", type=float, default=6.0)
    ap.add_argument("--gap", type=float, default=4.0)
    args = ap.parse_args()

    port = serial.Serial(args.port, args.baud, timeout=0.2)
    if not args.no_reset:
        # Both lines released: the processor runs. (RTS alone asserted holds
        # it in reset.)
        port.dtr = False
        port.rts = False
    start = time.time()
    pending = list(args.send)
    next_send = args.at
    while time.time() - start < args.seconds:
        data = port.read(4096)
        if data:
            sys.stdout.write(data.decode("utf-8", "replace"))
            sys.stdout.flush()
        if pending and time.time() - start >= next_send:
            port.write((pending.pop(0) + "\n").encode())
            next_send += args.gap
    port.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
