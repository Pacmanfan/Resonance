#!/usr/bin/env python3
"""Saves what the device's screen shows as a PNG.

    screenshot.py out.png [--port /dev/ttyUSB0] [--tap X Y ...] [--scale 2]

Sends the serial command "shot" (after any --tap, each followed by a pause),
which makes the firmware read the picture back from the panel, and writes
it as an image. The picture is what the panel holds, pixel for pixel; it is
not a photograph. Opening the port may restart the board, so the taps have
to lead from the Home screen. Needs pyserial and Pillow.
"""
import argparse
import sys
import time

import serial
from PIL import Image


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--tap", nargs=2, type=int, action="append", default=[], metavar=("X", "Y"))
    ap.add_argument("--settle", type=float, default=1.2, help="seconds to wait after each tap")
    ap.add_argument("--scale", type=int, default=1)
    ap.add_argument("--wait", type=float, default=9.0, help="seconds to wait after opening the port")
    args = ap.parse_args()

    port = serial.Serial()
    port.port, port.baudrate, port.timeout = args.port, args.baud, 0.5
    port.dtr = False
    port.rts = False
    port.open()
    # Opening the port can restart the board whatever the control lines are
    # set to; wait until it has started.
    time.sleep(args.wait)
    for x, y in args.tap:
        port.write(f"tap {x} {y}\n".encode())
        time.sleep(args.settle)
    port.reset_input_buffer()
    port.write(b"shot\n")

    image, pixels, rows = None, None, 0
    deadline = time.time() + 60
    while time.time() < deadline:
        line = port.readline().decode("ascii", "replace").strip()
        if not line.startswith("SHOT "):
            continue
        words = line.split()
        if words[1] == "begin":
            image = Image.new("RGB", (int(words[2]), int(words[3])))
            pixels = image.load()
        elif words[1] == "end":
            break
        elif image is not None:
            y, raw = int(words[1]), []
            for run in words[2:]:
                colour, _, count = run.partition("*")
                triple = (int(colour[0:2], 16), int(colour[2:4], 16), int(colour[4:6], 16))
                raw.extend([triple] * (int(count) if count else 1))
            if len(raw) != image.width or y >= image.height:
                print(f"row {y}: {len(raw)} pixels, expected {image.width}", file=sys.stderr)
                return 1
            # The panel's answer arrives one byte late: a triple holds the
            # green and blue of its pixel and the red of the next one. (Seen
            # on the ST7789 of the ESP32-2432S032; checked against the theme
            # colours in firmware/src/gui/theme.h.) The first pixel of a row
            # takes its red from its neighbour.
            for x in range(image.width):
                red = raw[x - 1][2] if x > 0 else raw[0][2]
                pixels[x, y] = (red, raw[x][0], raw[x][1])
            rows += 1
    if image is None or rows != image.height:
        print(f"incomplete picture: {rows} row(s)", file=sys.stderr)
        return 1
    if args.scale > 1:
        image = image.resize((image.width * args.scale, image.height * args.scale), Image.NEAREST)
    image.save(args.out)
    print(f"{args.out}: {image.width} x {image.height}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
