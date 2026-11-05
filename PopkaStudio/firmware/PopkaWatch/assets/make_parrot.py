#!/usr/bin/env python3
"""Popka, PopkaStudio's mascot: a little green parrot flapping its wings and singing — pixel art drawn here by code
(no image files from anywhere), saved as parrot.gif (big crisp pixels) for tools/halo_pixels.py:

    $PY PopkaStudio/firmware/PopkaWatch/assets/make_parrot.py [--preview OUT.png]
    $PY tools/halo_pixels.py --exact PopkaStudio/firmware/PopkaWatch/assets/parrot.gif \
        PopkaStudio/firmware/PopkaWatch/parrot.h parrot

The parrot is made of simple shapes on a grid of cells (ellipses, a wing turned about its shoulder), then outlined;
the notes and the twinkles around it are apart from it (the layer's "Notes": off, around it, all over the screen).
"""
import math
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 48, 40         # cells
CELL = 8              # px a cell in the GIF
FRAMES, MS = 8, 80    # one flap: 8 frames of 80 ms

COLORS = {
    ".": 0x000000,  # nothing (the screen's black)
    "K": 0x0E4A26,  # outline
    "G": 0x34B84A,  # body
    "L": 0x86E070,  # belly, light
    "D": 0x1F8A3B,  # shade
    "Y": 0xFFD23F,  # forehead
    "O": 0xFF9A2E,  # beak
    "o": 0xD96A12,  # beak, lower
    "B": 0x2F7BEA,  # wing tips, tail
    "C": 0x6FC8FF,  # tail stripe
    "R": 0xFF5E6C,  # cheek, crest
    "W": 0xFFFFFF,  # eye ring
    "E": 0x16181C,  # pupil
    "N": 0xFF7EB6,  # notes (apart)
    "T": 0xFFF3C4,  # twinkles (apart)
}
NOTE = ["..X.", "..XX", "..X.", "XXX.", "XXX."]
TWINKLE = [".X.", "XXX", ".X."]


def ellipse(g, cx, cy, rx, ry, c, where=None):
    y, x = np.mgrid[0:H, 0:W]
    m = ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1
    if where is not None:
        m &= where
    g[m] = c
    return m


def stamp(g, art, x0, y0, c):
    for j, row in enumerate(art):
        for i, ch in enumerate(row):
            x, y = x0 + i, y0 + j
            if ch == "X" and 0 <= x < W and 0 <= y < H:
                g[y, x] = c


def outline(g, mask):
    """Cells next to the mask (4 ways) that are empty — the outline."""
    m = np.zeros_like(mask)
    m[1:, :] |= mask[:-1, :]
    m[:-1, :] |= mask[1:, :]
    m[:, 1:] |= mask[:, :-1]
    m[:, :-1] |= mask[:, 1:]
    g[m & ~mask & (g == ".")] = "K"


def wing(g, angle, bob):
    """The wing: an ellipse turned about the shoulder (angle 0 — along the back to the tail, plus — up)."""
    sx, sy = 19.5, 19.5 + bob
    a = math.radians(angle)
    y, x = np.mgrid[0:H, 0:W]
    u, v = x + 0.5 - sx, y + 0.5 - sy
    # into the wing's own axes: along it (to the tip, at -x when angle is 0) and across
    along = -(u * math.cos(a) - v * math.sin(a))
    across = u * math.sin(a) + v * math.cos(a)
    m = ((along - 5.5) / 6.6) ** 2 + (across / 3.4) ** 2 <= 1
    g[m] = "D"
    g[m & (along > 8.2)] = "B"                    # the flight feathers' tips
    g[m & (along > 6.4) & (along <= 8.2)] = "C"
    g[m & (across < -1.6) & (along < 6.4)] = "G"  # its top edge catches the light
    edge = m & ~(np.roll(m, 1, 0) & np.roll(m, -1, 0) & np.roll(m, 1, 1) & np.roll(m, -1, 1))
    g[edge] = "K"


def frame(f):
    g = np.full((H, W), ".", dtype="<U1")
    bob = [0, 0, -1, -1, -1, 0, 0, 0][f]
    angles = [-62, -46, -18, 12, 30, 12, -18, -46]
    # tail: two long feathers down to the left
    for (ex, ey, c, r0) in ((-9.5, 12.5, "B", 2.0), (-11, 8.5, "C", 1.8)):
        for t in np.linspace(0, 1, 48):
            r = r0 - 1.0 * t
            ellipse(g, 12.5 + ex * t, 25.5 + bob + ey * t, r, r, c)
    body = ellipse(g, 19, 23 + bob, 9.6, 8.4, "G")
    head = ellipse(g, 25.5, 14.5 + bob, 6.8, 6.6, "G")
    fig = body | head
    ellipse(g, 21.5, 26.5 + bob, 6.2, 4.6, "L", fig)                       # belly
    shade = fig & ~(np.roll(np.roll(fig, -2, 1), -2, 0))                     # lower left edge in shade
    g[shade & (g != ".")] = "D"
    ellipse(g, 28.5, 11.5 + bob, 3.6, 2.6, "Y", head)                       # forehead
    # crest: three feathers
    for x, y, c in ((23, 7, "R"), (25, 6, "Y"), (27, 7, "R")):
        g[y + bob, x] = c
        g[y + 1 + bob, x] = c
    # beak: hooked, upper orange, lower darker
    for x, y in ((31, 14), (32, 14), (31, 15), (32, 15), (33, 15), (31, 16), (32, 16), (33, 16), (34, 16), (33, 17),
                 (34, 17), (34, 18)):
        g[y + bob, x] = "O"
    for x, y in ((31, 17), (32, 17), (31, 18)):
        g[y + bob, x] = "o"
    # eye: a white ring, the pupil, a highlight
    for x, y in ((26, 12), (27, 12), (25, 13), (28, 13), (25, 14), (28, 14), (26, 15), (27, 15)):
        g[y + bob, x] = "W"
    for x, y in ((26, 13), (27, 13), (26, 14), (27, 14)):
        g[y + bob, x] = "E"
    g[13 + bob, 26] = "W"
    g[17 + bob, 28] = g[17 + bob, 29] = "R"                                  # cheek
    # feet tucked under the belly
    for x in (20, 21, 24, 25):
        g[32 + bob, x] = "O"
    fig = g != "."
    outline(g, fig)
    wing(g, angles[f], bob)
    # notes rise from the beak, up and a little to the right, swaying; two of them, half the loop (8 cells) apart
    for i in range(2):
        ph = (f + i * 4) % FRAMES
        if ph < 7:
            stamp(g, NOTE, int(round(37 + 0.6 * ph)) + (1 if ph % 4 in (1, 2) else 0), 16 - 2 * ph + bob, "N")
    # twinkles blink on and off
    for (x, y, on) in ((5, 8, (0, 1, 2)), (41, 31, (4, 5, 6)), (8, 34, (2, 3)), (3, 19, (6, 7))):
        if f in on:
            stamp(g, TWINKLE, x, y, "T")
    return g


def to_image(g):
    rgb = np.zeros((H, W, 3), np.uint8)
    for ch, c in COLORS.items():
        rgb[g == ch] = ((c >> 16) & 255, (c >> 8) & 255, c & 255)
    return Image.fromarray(rgb).resize((W * CELL, H * CELL), Image.NEAREST)


def main():
    ims = [to_image(frame(f)) for f in range(FRAMES)]
    out = os.path.join(HERE, "parrot.gif")
    ims[0].save(out, save_all=True, append_images=ims[1:], duration=MS, loop=0, disposal=1, optimize=False)
    print(f"{out}: {FRAMES} frames of {W} × {H} cells")
    if "--preview" in sys.argv:
        path = sys.argv[sys.argv.index("--preview") + 1]
        sheet = Image.new("RGB", (W * CELL * 4 + 30, H * CELL * 2 + 10), (20, 20, 24))
        for f, im in enumerate(ims):
            sheet.paste(im, (f % 4 * (W * CELL + 10), f // 4 * (H * CELL + 10)))
        sheet.save(path)


if __name__ == "__main__":
    main()
