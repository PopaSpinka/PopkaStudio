"""Weather icons for HaloWatch: a sun partly hidden behind a cloud, as on the Pixel Watch weather tile.

Run from the repo root:  $PY projects/HaloWatch/assets/gen_icons.py
Writes assets/wx_big.png (the current-weather icon) and assets/wx_small.png (hourly row) for the Weather screen, the
other kinds in the same style (assets/wx_sun*.png, wx_cloud*.png, wx_rain*.png), and assets/face_*.png (partly cloudy,
sunny, cloudy, rain) for the watch face; then
`$PY tools/halo_assets.py projects/HaloWatch` turns them into Halo images.

Geometry is measured from design/weather_mockup.webp and converted to board pixels (circle 1254 px -> 466 px,
scale 0.3722): the sun is a circle; the cloud is two circles standing on its flat bottom, joined by a
base; a thin black gap separates the cloud from the sun. Shapes are drawn 8x larger and scaled down, so the edges are smooth.
The Weather screen now follows design/weather2_mockup.webp, keeping these icons: they are scaled so that the drawing is
as tall as the mockup's icons (BIG_SCALE, SMALL_SCALE).
"""
import math
import os
import re
import shlex
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import halo_assets as ha  # noqa: E402 — the same font rendering as the firmware's assets

HERE = os.path.dirname(os.path.abspath(__file__))
SS = 8  # supersampling
SCALE = 1.2  # the first weather layout: 1.2x the old mockup
# design/weather2_mockup.webp: the current-weather icon is 69.8 px tall, the hourly ones 45.9 px; the drawing of
# "sun behind a cloud" is 54 px tall at scale 1
WX2_SCALE = 1.2  # the Weather screen is drawn 1.2x the mockup (WX_SCALE in weather.h)
BIG_SCALE = 69.8 / 54 * WX2_SCALE
SMALL_SCALE = 45.9 / 54 * WX2_SCALE

SUN = 0xFCC425
CLOUD = 0xF9F4F8

# Big icon in board pixels: the cloud circles are fitted to the mockup's cloud mask (94.5 % overlap)
SUN_C, SUN_R = (125.8, 179.0), 22.7
BIG_C, BIG_R = (138.1, 195.8), 15.3      # right bump of the cloud
SMALL_C, SMALL_R = (119.8, 205.5), 9.7   # left bump, cut by the flat bottom
BASE_Y1 = 210.3                          # flat bottom of the cloud
GAP = 1.0                                # black gap between the cloud and the sun


def rgb(c):
    return (c >> 16 & 255, c >> 8 & 255, c & 255)


def cloud(size, ox, oy, s, grow):
    """Cloud mask: two circles standing on the flat bottom, joined by a base between their middles — round at both ends
    (2026-11-04: the left one used to be cut by the bottom, a corner like a piece cut off); grow > 0 makes it fatter
    (for the gap)."""
    m = Image.new("L", size, 0)
    d = ImageDraw.Draw(m)
    big, small = (BIG_C[0], BASE_Y1 - BIG_R), (SMALL_C[0], BASE_Y1 - SMALL_R)
    for c, r in ((big, BIG_R), (small, SMALL_R)):
        x, y, rr = (c[0] - ox) * s * SS, (c[1] - oy) * s * SS, (r + grow) * s * SS
        d.ellipse((x - rr, y - rr, x + rr, y + rr), fill=255)
    x0, x1 = (small[0] - ox) * s * SS, (big[0] - ox) * s * SS
    y0, y1 = (small[1] - oy) * s * SS, (BASE_Y1 + grow - oy) * s * SS
    d.rectangle((x0, y0, x1, y1), fill=255)
    return m


def icon(scale, path):
    save(icon_img(scale), path)


def save(img, path):
    img.save(path)
    print(f"{os.path.relpath(path)}: {img.width}x{img.height}, drawing {ink_h(img)} px tall")


def ink_h(img):
    """Height of what Halo will draw: rows with alpha above 0 after the 4-bit cut (as the image's ext rows)."""
    rows = np.nonzero((ha.q4(np.array(img)[..., 3]) > 0).any(1))[0]
    return int(rows.max() - rows.min() + 1) if len(rows) else 0


def digit_band(name, ref="14"):
    """How tall the digits of font NAME (from halo.conf) are on the board, by the flat ones ("1", "4": round digits
    overshoot a row up and down, as in every font): rows that survive the 4-bit cut — the big temperature's digits
    set the two lines the icon fits in (weather.h, wx_digit_rows — the same reference)."""
    conf = open(os.path.join(HERE, "gen_icons.conf"), encoding="utf-8").read()
    m = re.search(rf"^font\s+{name}\s+(.+)$", conf, re.M)
    parts = shlex.split(m.group(1).split("#")[0])
    path, size = ha.font_path(parts[0]), int(parts[1])
    axes = next((p[1:] for p in parts if p.startswith("@")), "")
    font = ImageFont.truetype(path, size)
    ha.set_axes(font, size, axes)
    top, bottom = 1e9, -1e9
    for ch in ref:
        a, x0, y0 = ha.render_glyph(font, ch)
        rows = np.nonzero((ha.q4(a) > 0).any(1))[0]
        top, bottom = min(top, y0 + rows.min()), max(bottom, y0 + rows.max() + 1)
    return bottom - top


def fit_height(make, target, scale):
    """Scale so that the drawing is exactly `target` px tall (each kind has its own shape and height): a first guess by
    the ratio, then halving between a scale that is too small and one that is too large."""
    img = make(scale)
    h = ink_h(img)
    if h == target:
        return img
    scale *= target / h
    lo, hi = scale * 0.97, scale * 1.03
    best = None
    for _ in range(24):
        mid = (lo + hi) / 2
        img = make(mid)
        h = ink_h(img)
        if h == target:
            best = img
            hi = mid  # the smallest scale that is tall enough: no row of the drawing goes past the line
        elif h < target:
            lo = mid
        else:
            hi = mid
    return best or img


def icon_img(scale):
    # Canvas around the icon with a margin, in board pixels relative to its top-left corner
    left, top = SUN_C[0] - SUN_R - 2, SUN_C[1] - SUN_R - 2
    right, bottom = BIG_C[0] + BIG_R + 2, BASE_Y1 + 2
    w, h = int((right - left) * scale + 1), int((bottom - top) * scale + 1)
    img = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    x, y, r = (SUN_C[0] - left) * scale * SS, (SUN_C[1] - top) * scale * SS, SUN_R * scale * SS
    d.ellipse((x - r, y - r, x + r, y + r), fill=rgb(SUN) + (255,))
    clear = Image.new("RGBA", img.size, (0, 0, 0, 0))
    img.paste(clear, (0, 0), cloud(img.size, left, top, scale, GAP))  # cut the gap out of the sun
    img.paste(Image.new("RGBA", img.size, rgb(CLOUD) + (255,)), (0, 0), cloud(img.size, left, top, scale, 0))
    return img.resize((w, h), Image.LANCZOS)


def weather_icon(kind, scale, path):
    save(weather_icon_img(kind, scale), path)


def weather_icon_img(kind, scale):
    """Sunny, cloudy and rain for the Weather screen: the canvas and the cloud of its "sun behind a cloud" (icon())."""
    left, top = SUN_C[0] - SUN_R - 2, SUN_C[1] - SUN_R - 2
    right, bottom = BIG_C[0] + BIG_R + 2, BASE_Y1 + 2
    w, h = int((right - left) * scale + 1), int((bottom - top) * scale + 1)
    img = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    mx, my = (left + right) / 2, (top + bottom) / 2  # canvas center, mockup px
    ccx = (SMALL_C[0] - SMALL_R + BIG_C[0] + BIG_R) / 2  # cloud bbox center, mockup px
    ccy = (BASE_Y1 - 2 * BIG_R + BASE_Y1) / 2

    def cloud_mask(dx, dy, k, grow):
        """The cloud scaled by k around its center, its center at the canvas center + (dx, dy)."""
        ox, oy = ccx - (mx + dx - left) / k, ccy - (my + dy - top) / k
        return cloud(img.size, ox, oy, scale * k, grow)

    def fill(color, mask):
        img.paste(Image.new("RGBA", img.size, rgb(color) + (255,)), (0, 0), mask)

    def cut(mask):
        img.paste(Image.new("RGBA", img.size, (0, 0, 0, 0)), (0, 0), mask)

    if kind == "sun":
        x, y, r = (mx - left) * scale * SS, (my - top) * scale * SS, 24 * scale * SS
        d.ellipse((x - r, y - r, x + r, y + r), fill=rgb(SUN) + (255,))
    elif kind == "cloud":  # a gray cloud behind a white one
        fill(FACE_CLOUD_BACK, cloud_mask(-7, -8, 0.82, 0))
        cut(cloud_mask(3, 5, 1, GAP))
        fill(CLOUD, cloud_mask(3, 5, 1, 0))
    elif kind == "snow":  # the weather widgets' fifth sky (wxwidgets.h): the rain's cloud, three flakes under it
        fill(CLOUD, cloud_mask(0, -7, 1, 0))
        for x, y in ((-10, 15), (0, 21), (10, 15)):
            xx, yy, rr = (mx + x - left) * scale * SS, (my + y - top) * scale * SS, 2.9 * scale * SS
            d.ellipse((xx - rr, yy - rr, xx + rr, yy + rr), fill=rgb(CLOUD) + (255,))
    elif kind == "rain":  # the cloud higher up, three slanted drops under it
        fill(CLOUD, cloud_mask(0, -7, 1, 0))
        for x in (-10, 0, 10):
            x0, y0 = (mx + x + 2.4 - left) * scale, (my + 13 - top) * scale
            x1, y1 = (mx + x - 1.8 - left) * scale, (my + 22 - top) * scale
            wd = 4.6 * scale
            d.line(((x0 * SS, y0 * SS), (x1 * SS, y1 * SS)), fill=rgb(FACE_RAIN) + (255,), width=int(wd * SS))
            for xx, yy in ((x0, y0), (x1, y1)):  # round caps
                d.ellipse(((xx - wd / 2) * SS, (yy - wd / 2) * SS, (xx + wd / 2) * SS, (yy + wd / 2) * SS),
                          fill=rgb(FACE_RAIN) + (255,))
    return img.resize((w, h), Image.LANCZOS)


# ---------- Watch face (design/face_mockup.webp, screen circle 1215 px -> 466 px) ----------
# Fitted to the mockup's masks: the cloud is a stadium (two caps of radius 7.19) plus one big circle on top (97 %
# overlap); the sun is a circle (97.6 %). Coordinates are relative to the icon's box center (193.43, 337.03) on the
# board; every face icon is drawn on the same canvas around that center, so they swap in place.
FACE_BOX = 54                      # canvas, px (the icon itself is about 48 x 48)
FACE_SCALE = 1.2                   # the face is drawn 1.2x the mockup (FACE_SCALE in face.h): icons are made that size
FACE_SUN = 0xFCC62E
FACE_CLOUD = 0xF8F7F9
FACE_CLOUD_BACK = 0xA9AEB5         # the far cloud of "cloudy"
FACE_RAIN = 0x7CC0FF
# Cloud relative to its own bbox center: stadium caps at x -14.63 .. 10.19, y 7.94, r 7.19; big circle (6.36, 0.32) r 15.46
CLOUD_CAPS = (-14.63, 10.19, 7.94, 7.19)
CLOUD_BIG = (6.36, 0.32, 15.46)
FACE_GAP = 1.0                     # transparent gap where one shape overlaps another


def face_cloud(size, cx, cy, s, grow):
    """Cloud mask centered at (cx, cy) canvas px, scaled by s; grow > 0 makes it fatter (for the gap)."""
    m = Image.new("L", size, 0)
    d = ImageDraw.Draw(m)
    xl, xr, yc, r = CLOUD_CAPS
    bx, by, br = CLOUD_BIG
    k = s * SS

    def disc(x, y, rr):
        d.ellipse(((cx + x * s) * SS - rr * k, (cy + y * s) * SS - rr * k,
                   (cx + x * s) * SS + rr * k, (cy + y * s) * SS + rr * k), fill=255)

    disc(xl, yc, r + grow / s)
    disc(xr, yc, r + grow / s)
    disc(bx, by, br + grow / s)
    d.rectangle(((cx + xl * s) * SS, (cy + (yc - r) * s - grow) * SS, (cx + xr * s) * SS, (cy + (yc + r) * s + grow) * SS),
                fill=255)
    return m


def face_icon(kind, path):
    n = FACE_BOX * SS
    img = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = FACE_BOX / 2

    def fill(color, mask):
        img.paste(Image.new("RGBA", img.size, rgb(color) + (255,)), (0, 0), mask)

    def cut(mask):
        img.paste(Image.new("RGBA", img.size, (0, 0, 0, 0)), (0, 0), mask)

    def sun(x, y, r):
        d.ellipse(((c + x - r) * SS, (c + y - r) * SS, (c + x + r) * SS, (c + y + r) * SS), fill=rgb(FACE_SUN) + (255,))

    if kind == "partly":  # the mockup: the sun upper left, the cloud in front of it lower right
        sun(-3.45, -3.68, 20.41)
        cut(face_cloud(img.size, c + 2.04, c + 8.95, 1, FACE_GAP))
        fill(FACE_CLOUD, face_cloud(img.size, c + 2.04, c + 8.95, 1, 0))
    elif kind == "sun":
        sun(0, 0, 21.5)
    elif kind == "cloud":  # a gray cloud behind a white one
        fill(FACE_CLOUD_BACK, face_cloud(img.size, c - 5.5, c - 9, 0.85, 0))
        cut(face_cloud(img.size, c + 2.2, c + 8.5, 1, FACE_GAP))
        fill(FACE_CLOUD, face_cloud(img.size, c + 2.2, c + 8.5, 1, 0))
    elif kind == "rain":  # the cloud higher up, three slanted drops under it
        fill(FACE_CLOUD, face_cloud(img.size, c, c - 6, 1, 0))
        for x in (-10, 0, 10):
            x0, y0, x1, y1 = c + x + 2.2, c + 13.5, c + x - 1.8, c + 21.5
            d.line(((x0 * SS, y0 * SS), (x1 * SS, y1 * SS)), fill=rgb(FACE_RAIN) + (255,), width=int(4.4 * SS))
            for x_, y_ in ((x0, y0), (x1, y1)):  # round caps
                d.ellipse(((x_ - 2.2) * SS, (y_ - 2.2) * SS, (x_ + 2.2) * SS, (y_ + 2.2) * SS), fill=rgb(FACE_RAIN) + (255,))
    out = round(FACE_BOX * FACE_SCALE)
    img = img.resize((out, out), Image.LANCZOS)
    img.save(path)
    print(f"{os.path.relpath(path)}: {out}x{out}")


# ---------- What to wear (the weather widget Wear, wxwidgets.h; web/mockups/weather-widgets.html variant 7) ----------
# Every one on the same canvas, 220 x 156 at scale 1, its point (0, 0) 110 px from the left and 90 from the top: the
# firmware puts the canvas' middle 12 px over that point, so they swap in place. Shapes — the mockup's SVG paths.
WEAR_W, WEAR_H, WEAR_OX, WEAR_OY = 220, 156, 110, 90
WEAR_TXT = 0xFFF8EE
WEAR_BODY = ("M-20,-52 L-50,-44 Q-60,-40 -63,-28 L-74,26 Q-75,32 -69,34 L-56,38 Q-50,39 -49,33 L-44,2 L-44,52 "
             "Q-44,58 -38,58 L38,58 Q44,58 44,52 L44,2 L49,33 Q50,39 56,38 L69,34 Q75,32 74,26 L63,-28 Q60,-40 50,-44 "
             "L20,-52 Q0,-36 -20,-52 Z")


def path_points(path, k):
    """An SVG path of M, L, Q (and Z) as points, the quadratic pieces sampled; k — pixels a unit."""
    pts, cur, toks = [], (0.0, 0.0), re.findall(r"[MLQZ]|-?[\d.]+", path)
    i, cmd = 0, None
    num = lambda: float(toks[i])  # noqa: E731
    while i < len(toks):
        if toks[i] in "MLQZ":
            cmd = toks[i]
            i += 1
            if cmd == "Z":
                continue
        if cmd in "ML":
            cur = (float(toks[i]), float(toks[i + 1]))
            i += 2
            pts.append(cur)
        elif cmd == "Q":
            c, e = (float(toks[i]), float(toks[i + 1])), (float(toks[i + 2]), float(toks[i + 3]))
            i += 4
            for j in range(1, 13):
                t = j / 12
                pts.append(((1 - t) ** 2 * cur[0] + 2 * (1 - t) * t * c[0] + t * t * e[0],
                            (1 - t) ** 2 * cur[1] + 2 * (1 - t) * t * c[1] + t * t * e[1]))
            cur = e
    return [((x + WEAR_OX) * k, (y + WEAR_OY) * k) for x, y in pts]


def wear_icon_img(kind, scale):
    """jacket, glasses, hoodie, umbrella or hat at this size (RGBA, the canvas above)."""
    k = scale * SS
    img = Image.new("RGBA", (int(WEAR_W * scale + 0.5) * SS, int(WEAR_H * scale + 0.5) * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    P = lambda x, y: ((x + WEAR_OX) * k, (y + WEAR_OY) * k)  # noqa: E731
    col = lambda c, a=255: rgb(c) + (a,)  # noqa: E731

    def line(pts, c, w, a=255):
        pts = [P(*q) for q in pts]
        d.line(pts, fill=col(c, a), width=max(1, int(w * k)), joint="curve")
        for q in (pts[0], pts[-1]):  # round ends
            r = w * k / 2
            d.ellipse((q[0] - r, q[1] - r, q[0] + r, q[1] + r), fill=col(c, a))

    def rrect(x, y, w, h, r, fill=None, outline=None, width=0):
        d.rounded_rectangle((*P(x, y), *P(x + w, y + h)), r * k, fill=col(fill) if fill else None,
                            outline=col(outline) if outline else None, width=int(width * k))

    def circle(x, y, r, c):
        (cx, cy) = P(x, y)
        d.ellipse((cx - r * k, cy - r * k, cx + r * k, cy + r * k), fill=col(c))

    if kind == "umbrella":
        pts = [P(64 * math.cos(math.pi * (1 - j / 48)), -56 * math.sin(math.pi * j / 48)) for j in range(49)]
        scallops = "M64,0 " + " ".join(f"Q{64 - 32 * i - 16},-11 {64 - 32 * (i + 1)},0" for i in range(4))
        d.polygon(pts + path_points(scallops, k)[1:], fill=col(0x5B9BFF))
        for dx in (-32, 0, 32):
            rib = path_points(f"M0,-56 Q{dx * 0.55},-40 {dx},0", k)
            d.line(rib, fill=col(0x2E5FB0), width=int(2 * k), joint="curve")
        line([(0, -56), (0, -66)], WEAR_TXT, 5)
        line([(0, 0), (0, 50)], WEAR_TXT, 6)
        (x0, y0), (x1, y1) = P(-22, 39), P(0, 61)
        d.arc((x0, y0, x1, y1), 0, 180, fill=col(WEAR_TXT), width=int(6 * k))
        line([(-22, 50), (-22, 50)], WEAR_TXT, 6)
    elif kind == "glasses":
        for i in range(9):
            a = math.radians(-80 + i * 20)
            line([(92 * math.sin(a), 30 - 92 * math.cos(a)), (106 * math.sin(a), 30 - 106 * math.cos(a))], SUN, 4, 140)
        for x in (-66, 10):
            rrect(x, -22, 56, 42, 17, fill=0x141414, outline=0xFFC94D, width=6)
        d.line(path_points("M-10,-8 Q0,-17 10,-8", k), fill=col(0xFFC94D), width=int(6 * k), joint="curve")
        line([(-66, -12), (-82, -19)], 0xFFC94D, 6)
        line([(66, -12), (82, -19)], 0xFFC94D, 6)
    elif kind in ("jacket", "hoodie"):
        hood, body, dark = kind == "hoodie", 0xAEB8C8 if kind == "hoodie" else 0xFFB36B, 0x6F7A8C if kind == "hoodie" else 0xB5703A
        if hood:
            d.polygon(path_points("M-28,-48 Q-34,-82 0,-84 Q34,-82 28,-48 Z", k), fill=col(dark))
        d.polygon(path_points(WEAR_BODY, k), fill=col(body))
        if hood:
            for sx in (-1, 1):
                line([(sx * 8, -42), (sx * 10, -16)], WEAR_TXT, 3)
                circle(sx * 10, -14, 3, WEAR_TXT)
            rrect(-26, 18, 52, 26, 9, outline=dark, width=3)
        else:
            line([(0, -40), (0, 58)], dark, 3)
            line([(-34, 24), (-16, 24)], dark, 3)
            line([(16, 24), (34, 24)], dark, 3)
            line([(-20, -52), (-8, -30)], dark, 3)
            line([(20, -52), (8, -30)], dark, 3)
    elif kind == "hat":
        d.polygon(path_points("M-56,10 Q-58,-62 0,-64 Q58,-62 56,10 Z", k), fill=col(0xFF7B7B))
        for dx in (-30, 0, 30):
            line([(dx * 0.9, -52 + abs(dx) * 0.3), (dx * 1.15, 4)], 0xE25F5F, 3)
        rrect(-62, 2, 124, 30, 12, fill=0xFFE4DF)
        for i in range(11):
            line([(-50 + i * 10, 9), (-50 + i * 10, 25)], 0xE8BDB6, 2.4)
        circle(0, -70, 15, WEAR_TXT)
    return img.resize((img.width // SS, img.height // SS), Image.LANCZOS)


if __name__ == "__main__":
    # The Weather screen's current icon: every kind exactly as tall as the big temperature's digits (w_now in
    # halo.conf) — it fits between the lines along their top and bottom (weather.h)
    band = digit_band("w_now")
    print(f"w_now digits: {band} px")
    save(fit_height(icon_img, band, BIG_SCALE), os.path.join(HERE, "wx_big.png"))
    icon(SMALL_SCALE, os.path.join(HERE, "wx_small.png"))
    for kind in ("sun", "cloud", "rain"):
        save(fit_height(lambda sc: weather_icon_img(kind, sc), band, BIG_SCALE), os.path.join(HERE, f"wx_{kind}.png"))
        weather_icon(kind, SMALL_SCALE, os.path.join(HERE, f"wx_{kind}_sm.png"))
    for kind in ("partly", "sun", "cloud", "rain"):
        face_icon(kind, os.path.join(HERE, f"face_{kind}.png"))
