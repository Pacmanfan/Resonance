#!/usr/bin/env python3
"""Send a sound file to the device's SD card over the serial port.

    serial_put.py <local file> <path below RESONANCE/AUDIO> [--port /dev/ttyUSB0]

A development aid: it uses the firmware's "put" command, at about 10 kB per
second, so it suits test files, not a music collection. Opening the port
restarts the board; the script waits for it to come up first.
"""
import argparse
import sys
import time

import serial


def wait_for(port, token, seconds):
    deadline = time.time() + seconds
    buffer = b""
    while time.time() < deadline:
        buffer += port.read(256)
        if token in buffer:
            return buffer
    raise SystemExit(f"serial_put: no '{token.decode()}' from the device; last output: {buffer[-200:]!r}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("local")
    ap.add_argument("remote")
    ap.add_argument("--port", default="/dev/ttyUSB0")
    args = ap.parse_args()
    data = open(args.local, "rb").read()

    port = serial.Serial(args.port, 115200, timeout=0.2)
    port.dtr = False
    port.rts = False
    wait_for(port, b"RADIO", 15)   # the start-up report has reached the radio
    time.sleep(4)
    port.reset_input_buffer()
    port.write(f"put {args.remote} {len(data)}\n".encode())
    wait_for(port, b"PUT ready", 5)
    sent = 0
    while sent < len(data):
        chunk = data[sent:sent + 4096]
        # Small writes with pauses: the device's receive buffer is 256 bytes.
        for i in range(0, len(chunk), 128):
            port.write(chunk[i:i + 128])
            port.flush()
            time.sleep(0.012)
        sent += len(chunk)
        # The last acknowledgement is followed at once by the result line.
        last = sent == len(data)
        out = wait_for(port, b"PUT stored" if last else f"PUT {sent}\n".encode(), 10)
    print(out.decode("utf-8", "replace").strip().splitlines()[-1])
    port.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
