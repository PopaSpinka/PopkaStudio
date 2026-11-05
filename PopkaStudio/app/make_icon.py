#!/usr/bin/env python3
"""PopkaStudio's app icon: the round AMOLED watch on a dark Mac icon tile, its time picked as in the editor — a blue
frame with handles around it. Drawn here (no image files): icon.png (1024) and icon.icns (build.sh puts it in the app).

    $PY PopkaStudio/app/make_icon.py
"""
import math
import os
import subprocess
import tempfile

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(HERE, "../../tools/fonts/GoogleSansFlex.ttf")
S = 2048  # drawn at twice the size, then made smaller: smooth edges
WARM = (255, 248, 238)
ACCENT = (76, 141, 255)


def squircle(size, box, n=5.0):
    """The Mac icon's tile: a superellipse (|x|^n + |y|^n = 1) — the corners round off smoothly, not as a circle's quarter"""
    x0, y0, x1, y1 = box
    cx, cy, rx, ry = (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2
    pts = []
    for i in range(720):
        t = 2 * math.pi * i / 720
        c, s = math.cos(t), math.sin(t)
        pts.append((cx + rx * math.copysign(abs(c) ** (2 / n), c), cy + ry * math.copysign(abs(s) ** (2 / n), s)))
    m = Image.new("L", (size, size), 0)
    ImageDraw.Draw(m).polygon(pts, fill=255)
    return m


def vgrad(size, top, bottom):
    g = Image.new("RGB", (1, 256))
    for y in range(256):
        k = y / 255
        g.putpixel((0, y), tuple(round(a + (b - a) * k) for a, b in zip(top, bottom)))
    return g.resize((size, size), Image.BILINEAR)


def disc(size, cx, cy, r):
    m = Image.new("L", (size, size), 0)
    ImageDraw.Draw(m).ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    return m


def font(px, wght):
    f = ImageFont.truetype(FONT, px)
    f.set_variation_by_axes([144, 100, wght, 100, 0])  # opsz, wdth, wght, ROND (rounded ends — as on the board), slnt
    return f


def draw():
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    # The tile (Mac icon grid: 824 of 1024, in the middle) with its shadow under it
    tile_box = (S * 100 / 1024, S * 100 / 1024, S * 924 / 1024, S * 924 / 1024)
    tile = squircle(S, tile_box)
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    shadow.putalpha(tile.point(lambda v: v * 0.45))
    shadow = ImageChops.offset(shadow, 0, S // 90).filter(ImageFilter.GaussianBlur(S / 60))
    img.alpha_composite(shadow)
    body = vgrad(S, (44, 46, 52), (14, 15, 17)).convert("RGBA")
    body.putalpha(tile)
    img.alpha_composite(body)
    # A thin light edge at the top of the tile (the glass of the Mac's icons)
    rim = Image.new("RGBA", (S, S), (255, 255, 255, 0))
    inner = squircle(S, tuple(v + d for v, d in zip(tile_box, (4, 6, -4, -2))))
    edge = ImageChops.subtract(tile, inner)
    fade = vgrad(S, (255, 255, 255), (0, 0, 0)).convert("L").point(lambda v: v * 0.35)
    rim.putalpha(ImageChops.multiply(edge, fade))
    img.alpha_composite(rim)

    cx = cy = S / 2
    # The watch: a metal bezel, the black screen in it
    R = S * 0.300
    bezel = vgrad(S, (98, 100, 108), (28, 29, 33)).convert("RGBA")
    bezel.putalpha(disc(S, cx, cy, R + S * 0.024))
    img.alpha_composite(bezel)
    ring = Image.new("RGBA", (S, S), (12, 12, 14, 255))
    ring.putalpha(disc(S, cx, cy, R + S * 0.006))
    img.alpha_composite(ring)
    screen = Image.new("RGBA", (S, S), (0, 0, 0, 255))
    screen.putalpha(disc(S, cx, cy, R))
    img.alpha_composite(screen)

    # On the screen: an arc along its edge (the accent, with its glow) — as the watch faces' arcs
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ar = R - S * 0.045
    box = (cx - ar, cy - ar, cx + ar, cy + ar)
    ImageDraw.Draw(glow).arc(box, 140, 400, fill=ACCENT + (200,), width=int(S * 0.022))
    img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(S / 70)))
    arc = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(arc)
    aw = int(S * 0.020)
    d.arc(box, 140, 400, fill=ACCENT + (255,), width=aw)
    w, rc = aw / 2, ar - aw / 2  # rounded ends: on the stroke's middle (the stroke goes inward from the box)
    for a in (140, 400):
        x, y = cx + rc * math.cos(math.radians(a)), cy + rc * math.sin(math.radians(a))
        d.ellipse((x - w, y - w, x + w, y + w), fill=ACCENT + (255,))
    img.alpha_composite(arc)

    # The time, warm white, picked as in the editor: a blue frame with four handles around it
    t = "10:09"
    f = font(int(S * 0.150), 660)
    l, tp, r, b = f.getbbox(t)
    tx, ty = cx - (l + r) / 2, cy - (tp + b) / 2 + S * 0.012
    tglow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(tglow).text((tx, ty), t, font=f, fill=WARM + (110,))
    img.alpha_composite(tglow.filter(ImageFilter.GaussianBlur(S / 90)))
    ImageDraw.Draw(img).text((tx, ty), t, font=f, fill=WARM + (255,))
    pad = S * 0.026
    fx0, fy0, fx1, fy1 = tx + l - pad, ty + tp - pad, tx + r + pad, ty + b + pad
    d = ImageDraw.Draw(img)
    lw = max(2, int(S * 0.0045))
    d.rectangle((fx0, fy0, fx1, fy1), outline=ACCENT + (255,), width=lw)
    h = S * 0.013
    for x, y in ((fx0, fy0), (fx1, fy0), (fx0, fy1), (fx1, fy1)):
        d.rectangle((x - h, y - h, x + h, y + h), fill=(255, 255, 255, 255), outline=ACCENT + (255,), width=lw)
    # The date above it, small and gray (as the faces' captions)
    small = font(int(S * 0.040), 620)
    s = "THU 5"
    l2, t2, r2, b2 = small.getbbox(s)
    d.text((cx - (l2 + r2) / 2, fy0 - S * 0.030 - b2), s, font=small, fill=(156, 157, 164, 255))
    return img.resize((1024, 1024), Image.LANCZOS)


def main():
    icon = draw()
    png = os.path.join(HERE, "icon.png")
    icon.save(png)
    with tempfile.TemporaryDirectory() as tmp:
        folder = os.path.join(tmp, "icon.iconset")
        os.mkdir(folder)
        for px in (16, 32, 128, 256, 512):
            icon.resize((px, px), Image.LANCZOS).save(os.path.join(folder, f"icon_{px}x{px}.png"))
            icon.resize((px * 2, px * 2), Image.LANCZOS).save(os.path.join(folder, f"icon_{px}x{px}@2x.png"))
        subprocess.run(["iconutil", "-c", "icns", folder, "-o", os.path.join(HERE, "icon.icns")], check=True)
    print(png, os.path.join(HERE, "icon.icns"))


if __name__ == "__main__":
    main()
