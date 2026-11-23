#!/usr/bin/env python3
"""Draws the project images in this directory: the README banner and the
GitHub social preview. They are drawings made by this script, not photographs
or CAD renders.

    python3 assets/make_images.py

Needs Pillow and the DejaVu Sans fonts.
"""
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = Path(__file__).resolve().parent
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

BACKGROUND = (8, 12, 22)
CYAN = (64, 224, 255)      # left ear
VIOLET = (176, 132, 255)   # right ear
TEXT = (236, 240, 248)
DIM = (128, 140, 164)


def wave(draw, box, cycles, colour, phase=0.0, width=3):
    x0, y0, x1, y1 = box
    mid, amp = (y0 + y1) / 2, (y1 - y0) / 2
    points = [(x, mid - amp * math.sin(2 * math.pi * cycles * (x - x0) / (x1 - x0) + phase))
              for x in range(x0, x1 + 1)]
    draw.line(points, fill=colour, width=width, joint="curve")


def picture(width, height, scale):
    image = Image.new("RGB", (width, height), BACKGROUND)
    # Concentric rings, as on the device's home screen, with a soft glow.
    glow = Image.new("RGB", (width, height), (0, 0, 0))
    g = ImageDraw.Draw(glow)
    cx, cy = int(width * 0.2), height // 2
    for i, radius in enumerate(range(int(40 * scale), int(200 * scale), int(26 * scale))):
        colour = CYAN if i % 2 == 0 else VIOLET
        shade = tuple(int(c * (0.75 - 0.09 * i)) for c in colour)
        g.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=shade, width=max(2, int(3 * scale)))
    # Two sines a few cycles apart: the two ears. Their difference is the beat.
    box = (int(width * 0.40), int(height * 0.60), int(width * 0.95), int(height * 0.86))
    wave(g, box, 10.0, tuple(int(c * 0.8) for c in CYAN), width=max(2, int(3 * scale)))
    wave(g, box, 11.0, tuple(int(c * 0.8) for c in VIOLET), width=max(2, int(3 * scale)))
    image = Image.blend(image, Image.eval(glow.filter(ImageFilter.GaussianBlur(10 * scale)), lambda v: min(255, v * 2)), 0.5)
    image = Image.composite(glow, image, glow.convert("L").point(lambda v: 255 if v > 24 else 0))

    d = ImageDraw.Draw(image)
    title = ImageFont.truetype(FONT_BOLD, int(84 * scale))
    line = ImageFont.truetype(FONT, int(26 * scale))
    small = ImageFont.truetype(FONT, int(19 * scale))
    x = int(width * 0.40)
    d.text((x, int(height * 0.16)), "RESONANCE", font=title, fill=TEXT)
    d.text((x, int(height * 0.16) + int(104 * scale)), "Open-source binaural meditation device", font=line, fill=DIM)
    d.text((x, int(height * 0.16) + int(142 * scale)), "ESP32  ·  Bluetooth audio  ·  GPL-3.0", font=small, fill=DIM)
    return image


if __name__ == "__main__":
    picture(1600, 480, 1.0).save(HERE / "banner.png", optimize=True)
    picture(1280, 640, 1.15).save(HERE / "social-preview.png", optimize=True)
    print("wrote banner.png and social-preview.png")
