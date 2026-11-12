"""Ресурсы для библиотеки Halo: шрифты, одноцветные значки и цветные картинки — в форматы Halo, пиксель в пиксель.

Использование (из корня репозитория):
  $PY tools/halo_assets.py experiments/HaloFirst
Читает <скетч>/assets/halo.conf, пишет <скетч>/src/halo_assets.h и .cpp. Превью — build/halo_assets/<скетч>/.

halo.conf — по строке на ресурс (# — комментарий; пути шрифтов — от tools/fonts, картинок — от папки скетча):
  font  ИМЯ  ФАЙЛ.ttf  РАЗМЕР_PX  НАБОР…       # набор: digits latin cyrillic punct, "свои буквы" или коды 0xF004
                                              # (значки Font Awesome шрифтом: ставятся по строке, как буквы)
                                              # переменный шрифт: @wght=500,ROND=100 — оси (opsz по умолчанию = кегль)
                                              # запечь эффект в буквы: +glow=3:12 — свечение (два размытия, как
                                              # у неона); +shadow=64:0.85 — длинная тень вниз-вправо на 64 px (яркость
                                              # от буквы к концу тени падает со степенью 0,85); рисуется под буквами
  icon  ИМЯ  ФАЙЛ.ttf  РАЗМЕР_PX  0xКОД         # значок из шрифта (Font Awesome) → маска, красится при рисовании
  mask  ИМЯ  ФАЙЛ.png  [ШИРИНА]                 # одноцветный значок из картинки (берётся прозрачность)
  image ИМЯ  ФАЙЛ.png  [ШИРИНА]                 # цветная картинка

Почему так (а не SVG на плате): разбирать кривые и сглаживать их в каждом кадре дорого. На Mac FreeType и Pillow
рисуют букву или значок ровно нужного размера со сглаживанием; на плате остаётся скопировать готовые пиксели.
Форматы (lib/Halo/src/Halo.h):
  маска (буква, значок) — 4 бита на пиксель, строка (w+1)/2 байт; у строки — первый и последний непустой столбец;
  картинка — цвет × прозрачность в RGB565 (порядок дисплея) + та же 4-битная прозрачность.
"""
import os
import re
import shlex
import sys
import urllib.request

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, "tools/fonts")

SETS = {
    "digits": "0123456789",
    "latin": "".join(chr(c) for c in range(0x20, 0x7F)),
    "cyrillic": "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя",
    "punct": " .,:;!?-–—+−%°«»…·/()",
}


def q4(a):
    """0..255 → 0..15 с округлением."""
    return np.clip((a.astype(np.int32) * 15 + 127) // 255, 0, 15).astype(np.uint8)


def pack4(a4):
    """Строки по (w+1)/2 байт, левый пиксель — старшие 4 бита; и ext: первый/последний непустой столбец строки."""
    h, w = a4.shape
    pad = np.zeros((h, w + (w & 1)), np.uint8)
    pad[:, :w] = a4
    bits = (pad[:, 0::2] << 4) | pad[:, 1::2]
    ext = []
    for row in a4:
        nz = np.nonzero(row)[0]
        # (a byte each: a mask wider than 256 — a huge digit, a glow — starts its scan by 254 at most and ends "at the end
        # of the row", 255, as lib/Halo reads it)
        ext += [min(int(nz[0]), 254), int(nz[-1]) if nz[-1] < 255 else 255] if len(nz) else [1, 0]
    return bits.flatten().tolist(), ext


def render_glyph(font, ch):
    x0, y0, x1, y1 = font.getbbox(ch, anchor="ls")
    w, h = max(1, x1 - x0), max(1, y1 - y0)
    img = Image.new("L", (w, h), 0)
    ImageDraw.Draw(img).text((-x0, -y0), ch, font=font, fill=255, anchor="ls")
    return np.array(img), x0, y0


def trim(a):
    """Обрезать пустые края маски: (маска, сдвиг x, сдвиг y)."""
    ys, xs = np.nonzero(a)
    if not len(ys):
        return np.zeros((1, 1), np.uint8), 0, 0
    return a[ys.min():ys.max() + 1, xs.min():xs.max() + 1], int(xs.min()), int(ys.min())


class Out:
    def __init__(self):
        self.h, self.c = [], []

    def arr(self, ctype, name, data, per_line=24):
        lines = []
        for i in range(0, len(data), per_line):
            lines.append("  " + ", ".join(str(v) for v in data[i:i + per_line]) + ",")
        self.c.append(f"static const {ctype} {name}[] = {{\n" + "\n".join(lines) + "\n};")


# Шрифты не хранятся в git (tools/fonts/ — в .gitignore): нет файла — скачать из первоисточника (все — лицензия OFL)
GSF = "https://raw.githubusercontent.com/google/fonts/main/ofl/googlesansflex/"
KNOWN_FONTS = {
    "GoogleSansFlex.ttf": GSF + "GoogleSansFlex%5BROND%2Copsz%2Cslnt%2Cwdth%2Cwght%5D.ttf",
    "GoogleSansFlex-OFL.txt": GSF + "OFL.txt",
    "Montserrat-Medium.ttf": "https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-Medium.ttf",
    "Montserrat-SemiBold.ttf": "https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-SemiBold.ttf",
    "fa-solid-900.ttf": "https://raw.githubusercontent.com/FortAwesome/Font-Awesome/6.x/webfonts/fa-solid-900.ttf",
    # PopkaStudio's clocks (Layers → Clock: Serif, Terminal), Google Fonts, OFL
    "InstrumentSerif-Regular.ttf": "https://raw.githubusercontent.com/google/fonts/main/ofl/instrumentserif/InstrumentSerif-Regular.ttf",
    "InstrumentSerif-Italic.ttf": "https://raw.githubusercontent.com/google/fonts/main/ofl/instrumentserif/InstrumentSerif-Italic.ttf",
    "InstrumentSerif-OFL.txt": "https://raw.githubusercontent.com/google/fonts/main/ofl/instrumentserif/OFL.txt",
    "JetBrainsMono.ttf": "https://raw.githubusercontent.com/google/fonts/main/ofl/jetbrainsmono/JetBrainsMono%5Bwght%5D.ttf",
    "JetBrainsMono-OFL.txt": "https://raw.githubusercontent.com/google/fonts/main/ofl/jetbrainsmono/OFL.txt",
}
LICENSES = {"GoogleSansFlex.ttf": "GoogleSansFlex-OFL.txt", "InstrumentSerif-Regular.ttf": "InstrumentSerif-OFL.txt",
            "InstrumentSerif-Italic.ttf": "InstrumentSerif-OFL.txt", "JetBrainsMono.ttf": "JetBrainsMono-OFL.txt"}


def font_path(name):
    path = os.path.join(FONTS, name)
    if not os.path.exists(path) and name in KNOWN_FONTS:
        os.makedirs(FONTS, exist_ok=True)
        print(f"  downloading {name}")
        urllib.request.urlretrieve(KNOWN_FONTS[name], path)
        lic = LICENSES.get(name)
        if lic and not os.path.exists(os.path.join(FONTS, lic)):
            urllib.request.urlretrieve(KNOWN_FONTS[lic], os.path.join(FONTS, lic))
    return path


AXES = {"opsz": "Optical Size", "wdth": "Width", "wght": "Weight", "ROND": "Roundness", "slnt": "Slant"}


def set_axes(font, size, spec):
    """Переменный шрифт (Google Sans Flex): оси из строки "wght=500,ROND=100"; оптический размер — по кеглю."""
    try:
        axes = font.get_variation_axes()
    except Exception:
        return
    want = {"opsz": size}
    for part in filter(None, spec.split(",")):
        k, v = part.split("=")
        want[k.strip()] = float(v)
    vals = []
    for a in axes:
        name = a["name"].decode() if isinstance(a["name"], bytes) else a["name"]
        tag = next((t for t, n in AXES.items() if n == name), None)
        v = want.get(tag, a["default"])
        vals.append(min(max(v, a["minimum"]), a["maximum"]))
    font.set_variation_by_axes(vals)


def glow(a, sd1, sd2):
    """Свечение буквы: два размытия (sd1 — тесное, sd2 — широкое), сложенные как слои SVG feMerge (широкое, тесное,
    тесное — «поверх»). Поле вокруг — 3·sd2. → (маска, сдвиг поля)."""
    pad = int(np.ceil(3 * sd2))
    im = Image.fromarray(np.pad(a, pad))
    b1 = np.asarray(im.filter(ImageFilter.GaussianBlur(sd1)), np.float32) / 255
    b2 = np.asarray(im.filter(ImageFilter.GaussianBlur(sd2)), np.float32) / 255
    g = 1 - (1 - b2) * (1 - b1) ** 2
    return np.clip(np.round(g * 255), 0, 255).astype(np.uint8), pad


def long_shadow(a, n, power):
    """Длинная тень вниз-вправо: копии буквы со сдвигом d = 1…n по диагонали, ближняя поверх дальней; яркость копии —
    1 − (d/n)^power. → маска того же левого верхнего угла, шире и выше на n."""
    h, w = a.shape
    src = a.astype(np.float32) / 255
    acc = np.zeros((h + n, w + n), np.float32)
    for d in range(1, n + 1):
        sl = acc[d:d + h, d:d + w]
        np.maximum(sl, src * (1 - (d / n) ** power), out=sl)
    return np.clip(np.round(acc * 255), 0, 255).astype(np.uint8)


BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]], np.float32) / 16 - 15 / 32


def dither(a):
    """Плавный спад (свечение, тень) в 16 уровней маски — узором 4×4, а не ступеньками (кольца на широком свечении)."""
    h, w = a.shape
    b = np.tile(BAYER4, ((h + 3) // 4, (w + 3) // 4))[:h, :w]
    soft = (a > 0) & (a < 255)
    out = a.astype(np.float32) + b * (255 / 15) * soft
    return np.clip(np.round(out), 0, 255).astype(np.uint8)


def do_font(out, name, path, size, chars, preview, axes="", fx=""):
    font = ImageFont.truetype(path, size)
    set_axes(font, size, axes)
    ascent, descent = font.getmetrics()
    line = ascent + descent
    codes = sorted(set(ord(c) for c in chars))
    bits, ext, glyphs, missing = [], [], [], []
    sheet = []
    for cp in codes:
        ch = chr(cp)
        if ch != " " and not font.getmask(ch).getbbox():
            missing.append(ch)
            continue
        a, x0, y0 = render_glyph(font, ch)
        a, tx, ty = trim(a)
        if fx.startswith("glow=") and a.any():
            sd1, sd2 = (float(v) for v in fx[5:].split(":"))
            a, pad = glow(a, sd1, sd2)
            a, tx2, ty2 = trim(dither(a))
            tx, ty = tx - pad + tx2, ty - pad + ty2
        elif fx.startswith("shadow=") and a.any():
            n, power = fx[7:].split(":")
            a, tx2, ty2 = trim(dither(long_shadow(a, int(n), float(power))))
            tx, ty = tx + tx2, ty + ty2
        a4 = q4(a)
        b, e = pack4(a4)
        adv = int(round(font.getlength(ch)))
        glyphs.append((cp, a4.shape[1], a4.shape[0], x0 + tx, ascent + y0 + ty, adv, len(bits), len(ext)))
        bits += b
        ext += e
        sheet.append(a)
    out.arr("uint8_t", f"{name}_bits", bits)
    out.arr("uint8_t", f"{name}_ext", ext)
    out.c.append(f"static const uint32_t {name}_codes[] = {{{', '.join(str(g[0]) for g in glyphs)}}};")
    rows = [f"  {{{w}, {h}, {ox}, {oy}, {adv}, {name}_bits + {bo}, {name}_ext + {eo}}},"
            for cp, w, h, ox, oy, adv, bo, eo in glyphs]
    out.c.append(f"static const HaloMask {name}_glyphs[] = {{\n" + "\n".join(rows) + "\n};")
    out.c.append(f"const HaloFont {name} = {{{size}, {line}, {ascent}, {len(glyphs)}, {name}_codes, {name}_glyphs}};")
    out.h.append(f"extern const HaloFont {name};  // {os.path.basename(path)} {size} px, {len(glyphs)} букв, "
                 f"строка {line}, базовая линия {ascent}")
    if missing:
        print(f"  {name}: letters not in the font: {''.join(missing)}")
    # Превью: строка всех букв
    W = sum(a.shape[1] + 4 for a in sheet) + 4
    H = max((a.shape[0] for a in sheet), default=1) + 8
    im = Image.new("L", (min(W, 4000), H), 0)
    x = 4
    for a in sheet:
        if x + a.shape[1] > im.width:
            break
        im.paste(Image.fromarray(a), (x, 4))
        x += a.shape[1] + 4
    im.save(os.path.join(preview, f"font_{name}.png"))
    return len(bits) + len(ext) + len(glyphs) * 20


def mask_out(out, name, a, preview, note):
    a, _, _ = trim(a)
    a4 = q4(a)
    b, e = pack4(a4)
    out.arr("uint8_t", f"{name}_bits", b)
    out.arr("uint8_t", f"{name}_ext", e)
    out.c.append(f"const HaloMask {name} = {{{a4.shape[1]}, {a4.shape[0]}, 0, 0, {a4.shape[1]}, {name}_bits, {name}_ext}};")
    out.h.append(f"extern const HaloMask {name};  // {note}, {a4.shape[1]}×{a4.shape[0]}")
    Image.fromarray(a).save(os.path.join(preview, f"mask_{name}.png"))
    return len(b) + len(e)


def do_icon(out, name, path, size, code, preview):
    font = ImageFont.truetype(path, size)
    a, _, _ = render_glyph(font, chr(code))
    if not a.any():
        sys.exit(f"{name}: no icon {hex(code)} in {os.path.basename(path)}")
    return mask_out(out, name, a, preview, f"{os.path.basename(path)} {hex(code)} {size} px")


def load_png(path, width):
    im = Image.open(path).convert("RGBA")
    if width:
        im = im.resize((width, max(1, round(im.height * width / im.width))), Image.LANCZOS)
    return np.array(im)


def do_image(out, name, path, width, preview):
    rgba = load_png(path, width)
    a = rgba[:, :, 3]
    ys, xs = np.nonzero(a)
    rgba = rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    a = rgba[:, :, 3].astype(np.float32) / 255
    rgb = rgba[:, :, :3].astype(np.float32) * a[:, :, None]  # цвет × прозрачность («как на чёрном»)
    r = np.clip(np.round(rgb[:, :, 0] * 31 / 255), 0, 31).astype(np.uint16)
    g = np.clip(np.round(rgb[:, :, 1] * 63 / 255), 0, 63).astype(np.uint16)
    b = np.clip(np.round(rgb[:, :, 2] * 31 / 255), 0, 31).astype(np.uint16)
    px = (r << 11) | (g << 5) | b
    if not os.environ.get("HALO_PX_LE"):  # (плата, у которой кадр читается как есть, — HALO_PX_LE=1: порядок памяти)
        px = ((px & 0xFF) << 8) | (px >> 8)  # порядок дисплея: старший байт первым
    a4 = q4(rgba[:, :, 3])
    px[a4 == 0] = 0
    bits, ext = pack4(a4)
    h, w = a4.shape
    out.arr("uint16_t", f"{name}_px", px.flatten().tolist(), 16)
    out.arr("uint8_t", f"{name}_alpha", bits)
    out.arr("uint8_t", f"{name}_ext", ext)
    out.c.append(f"const HaloImage {name} = {{{w}, {h}, {name}_px, {name}_alpha, {name}_ext}};")
    out.h.append(f"extern const HaloImage {name};  // {os.path.basename(path)}, {w}×{h}")
    Image.fromarray(rgba).save(os.path.join(preview, f"image_{name}.png"))
    return w * h * 2 + len(bits) + len(ext)


def do_line(out, sketch, raw, preview):
    """Одна строка halo.conf → в out. Возвращает байты во флеше (None — пустая строка или комментарий).
    Отдельно от main(): так же строку разбирает PopkaStudio (шрифты для браузера — тем же кодом, что и для платы)."""
    p = shlex.split(raw, comments=True)  # # — комментарий (внутри кавычек — обычный знак)
    if not p:
        return None
    kind, name = p[0], p[1]
    if kind == "font":
        chars, axes, fx = "", "", ""
        for s in p[4:]:
            if s.startswith("@"):
                axes = s[1:]
            elif re.fullmatch(r"\+(glow|shadow)=[\d.:]+", s):
                fx = s[1:]
            else:
                chars += chr(int(s, 16)) if re.fullmatch(r"0x[0-9a-fA-F]+", s) else SETS.get(s, s)
        return do_font(out, name, font_path(p[2]), int(p[3]), chars, preview, axes, fx)
    if kind == "icon":
        return do_icon(out, name, font_path(p[2]), int(p[3]), int(p[4], 16), preview)
    if kind == "mask":
        a = load_png(os.path.join(sketch, p[2]), int(p[3]) if len(p) > 3 else 0)[:, :, 3]
        return mask_out(out, name, a, preview, os.path.basename(p[2]))
    if kind == "image":
        return do_image(out, name, os.path.join(sketch, p[2]), int(p[3]) if len(p) > 3 else 0, preview)
    sys.exit(f"cannot read the line: {raw}")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    sketch = os.path.abspath(sys.argv[1])
    conf = os.path.join(sketch, "assets/halo.conf")
    preview = os.path.join(ROOT, "build/halo_assets", os.path.basename(sketch))
    os.makedirs(preview, exist_ok=True)
    out = Out()
    total = 0
    for raw in open(conf, encoding="utf-8"):
        n = do_line(out, sketch, raw, preview)
        if n is None:
            continue
        kind, name = shlex.split(raw, comments=True)[:2]
        total += n
        print(f"  {kind:5} {name:14} {n / 1024:6.1f} KB")
    src = os.path.join(sketch, "src")
    os.makedirs(src, exist_ok=True)
    head = "// Сгенерировано tools/halo_assets.py из assets/halo.conf — руками не править.\n"
    with open(os.path.join(src, "halo_assets.h"), "w") as f:
        f.write(head + "#pragma once\n#include <Halo.h>\n\n" + "\n".join(out.h) + "\n")
    with open(os.path.join(src, "halo_assets.cpp"), "w") as f:
        f.write(head + '#include "halo_assets.h"\n\n' + "\n\n".join(out.c) + "\n")
    print(f"in all {total / 1024:.0f} KB of flash → {os.path.relpath(src, ROOT)}/halo_assets.*, previews — "
          f"{os.path.relpath(preview, ROOT)}/")


if __name__ == "__main__":
    main()
