#!/usr/bin/env python3
"""PopkaStudio's app icon: Popka, the studio's parrot (firmware/PopkaWatch/assets/make_parrot.py, in cells as on the
watch), picked as in the editor — a blue frame with handles around it and its size under it — on a graphite Mac icon tile.
Drawn here (no image files): icon.png (1024) and icon.icns (build.sh puts it in the app). The other ideas it was chosen
from — studio/web/mockups/icons.py (variant 7).

    $PY studio/app/make_icon.py
"""
import importlib.util
import os
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(HERE, "../../tools/fonts/GoogleSansFlex.ttf")
PARROT = os.path.join(HERE, "../../firmware/PopkaWatch/assets/make_parrot.py")
S = 2048  # drawn at twice the size, then made smaller: smooth edges
ACCENT = (76, 141, 255)
TILE_TOP, TILE_BOTTOM = (78, 82, 92), (42, 44, 50)  # graphite: lighter than the watch's black, the blue frame still bright


def tile_mask(size, box):
    """The Mac icon's tile as macOS 26+ expects it: Apple's grid (824 of 1024 in the middle) with its corner (185 of
    1024). A tile of another shape (our old superellipse) macOS puts small on a gray plate — the "legacy icon" look"""
    m = Image.new("L", (size, size), 0)
    ImageDraw.Draw(m).rounded_rectangle(box, radius=size * 185.4 / 1024, fill=255)
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


def parrot():
    """Popka's first frame (wings up) without its notes and twinkles, one px a cell, cut to what is drawn"""
    spec = importlib.util.spec_from_file_location("make_parrot", PARROT)
    mp = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mp)
    g = mp.frame(0)
    a = np.zeros((mp.H, mp.W, 4), np.uint8)
    for ch, c in mp.COLORS.items():
        if ch not in ".NT":
            a[g == ch] = ((c >> 16) & 255, (c >> 8) & 255, c & 255, 255)
    im = Image.fromarray(a, "RGBA")
    return im.crop(im.getbbox())


def draw():
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    # The tile (Mac icon grid: 824 of 1024, in the middle) with its shadow under it
    tile_box = (S * 100 / 1024, S * 100 / 1024, S * 924 / 1024, S * 924 / 1024)
    tile = tile_mask(S, tile_box)
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    shadow.putalpha(tile.point(lambda v: v * 0.45))
    shadow = ImageChops.offset(shadow, 0, S // 90).filter(ImageFilter.GaussianBlur(S / 60))
    img.alpha_composite(shadow)
    body = vgrad(S, TILE_TOP, TILE_BOTTOM).convert("RGBA")
    body.putalpha(tile)
    img.alpha_composite(body)
    # A thin light edge at the top of the tile (the glass of the Mac's icons)
    rim = Image.new("RGBA", (S, S), (255, 255, 255, 0))
    inner = tile_mask(S, tuple(v + d for v, d in zip(tile_box, (4, 6, -4, -2))))
    edge = ImageChops.subtract(tile, inner)
    fade = vgrad(S, (255, 255, 255), (0, 0, 0)).convert("L").point(lambda v: v * 0.35)
    rim.putalpha(ImageChops.multiply(edge, fade))
    img.alpha_composite(rim)

    # Popka in its cells (big crisp pixels), half the icon wide, a little above the middle
    p = parrot()
    cell = int(S * 0.50 / p.width)
    p = p.resize((p.width * cell, p.height * cell), Image.NEAREST)
    x, y = round(S / 2 - p.width / 2), round(S * 0.47 - p.height / 2)
    img.alpha_composite(p, (x, y))

    # Picked as in the editor: a blue frame with four handles around it
    d = ImageDraw.Draw(img)
    pad = S * 0.030
    fx0, fy0, fx1, fy1 = x - pad, y - pad, x + p.width + pad, y + p.height + pad
    lw = max(2, int(S * 0.0075))
    d.rectangle((fx0, fy0, fx1, fy1), outline=ACCENT + (255,), width=lw)
    h = S * 0.016
    for hx, hy in ((fx0, fy0), (fx1, fy0), (fx0, fy1), (fx1, fy1)):
        d.rectangle((hx - h, hy - h, hx + h, hy + h), fill=(255, 255, 255, 255), outline=ACCENT + (255,), width=lw)
    # Its size under it, in a blue pill (make_parrot's grid: 48 × 40 cells)
    f = font(int(S * 0.044), 660)
    s = "48 × 40"
    l, t, r, b = f.getbbox(s)
    cx, cy = S / 2, fy1 + S * 0.075
    pw, ph = (r - l) / 2 + S * 0.024, (b - t) / 2 + S * 0.016
    d.rounded_rectangle((cx - pw, cy - ph, cx + pw, cy + ph), ph, fill=ACCENT + (255,))
    d.text((cx - (l + r) / 2, cy - (t + b) / 2), s, font=f, fill=(255, 255, 255, 255))
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
