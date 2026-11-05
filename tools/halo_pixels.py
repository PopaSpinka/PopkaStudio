"""A GIF of big pixels (pixel art scaled up, its edges maybe blurred by the scaling) → lib/Halo's HaloPixels: cells.

Usage (from the repository's root):
  $PY tools/halo_pixels.py IN.gif OUT.h NAME [--preview OUT.png] [--exact] [--cell PX]

--exact: the GIF is crisp (drawn by code, not scaled up with blur) — every color in it is real, none is merged.
--cell PX: the cells are PX × PX pixels from the top left corner (a GIF drawn by code) — the grid is not looked for.

What it does:
  * finds the cell grid by itself, along x and along y: where colors change, the changes fall on the grid lines — the
    pitch and the offset that put the most changes on lines (fewer lines win: a half pitch would fit too);
  * a cell's color — the color most of its middle pixels have (the blurred edges are left out), every frame;
  * the colors: the background (the commonest) becomes 0 — "nothing", not drawn; colors that only a few cells have
    (left from the blur) go to the nearest real one; up to 15 colors;
  * the subject — the biggest connected part of every frame (a character and its trail) — apart from what is around it
    (sparkles): those cells get colors of their own (NAME_APART — the bits to hide them by: HaloItem.hide), also where
    a sparkle crosses the subject: a cell of a sparkle's color between cells of the subject's other colors;
  * NAME.ax, ay — the middle of the subject (the item stands and turns by it), NAME_body — the subject's cells;
  * the trail: if the subject's leftmost columns repeat (a wavy rainbow behind it), the period and how many
    columns from the left edge do — the item draws it as long as asked (HaloItem.trail);
  * the sparkles of every frame as a list of their cells (NAME_sparks, by row): Halo draws them one by one instead
    of going over the empty sky — what is around the subject costs as much as there is of it.

Out: a header with the palette, the cells (4 bits, rows of (w+1)/2 bytes, the left cell in the high bits), the frames
one after another and how long a frame lasts. It all stays in flash: a 69 × 68 GIF of 12 frames — 28 KB.
"""
import sys
from collections import Counter, deque

import numpy as np
from PIL import Image, ImageSequence


def frames_of(path):
    im = Image.open(path)
    out, ms = [], []
    for f in ImageSequence.Iterator(im):
        out.append(np.array(f.convert("RGB")).astype(np.int32))
        ms.append(f.info.get("duration", 100) or 100)
    return out, ms


def fit_axis(frames, axis):
    """The grid along one axis: (pitch, offset of the first line), from where colors change."""
    size = frames[0].shape[1 - axis]
    edges = np.zeros(size)
    for f in frames:
        d = np.any(np.diff(f, axis=1 - axis) != 0, axis=2)  # a change between pixel k-1 and k
        edges[1:] += d.sum(axis=axis)
    total = edges.sum() or 1
    pos = np.arange(size, dtype=float)
    sigma = 0.45

    def search(pitches, step):
        best = None
        for p in pitches:
            base = min(0.999, sigma * np.sqrt(np.pi) / p)  # what a grid of this pitch catches by chance
            for o in np.arange(0, p, step(p)):
                d = np.abs((pos - o + p / 2) % p - p / 2)
                score = ((edges * np.exp(-(d / sigma) ** 2)).sum() / total - base) / (1 - base)
                if best is None or score > best[0]:
                    best = (score, p, o)
        return best
    _, p, _ = search(np.arange(1.5, min(32, size / 4), 0.05), lambda p: p / 20)  # roughly, then finely
    _, p, o = search(np.arange(p - 0.06, p + 0.06, 0.002), lambda p: 0.05)
    if o > p / 2:  # a part of a cell before the first line: it is a cell too
        o -= p
    return p, o, int(round((size - o) / p))


def cells_of(f, px, ox, nx, py, oy, ny):
    """Each cell's color: the commonest among its middle pixels."""
    h, w = f.shape[:2]
    keys = (f[..., 0] << 16) | (f[..., 1] << 8) | f[..., 2]
    out = np.zeros((ny, nx), np.int64)
    for j in range(ny):
        cy = oy + (j + 0.5) * py
        y0, y1 = max(0, int(cy - py * 0.3)), min(h, int(cy + py * 0.3) + 1)
        for i in range(nx):
            cx = ox + (i + 0.5) * px
            x0, x1 = max(0, int(cx - px * 0.3)), min(w, int(cx + px * 0.3) + 1)
            out[j, i] = Counter(keys[y0:y1, x0:x1].ravel().tolist()).most_common(1)[0][0]
    return out


def rgb(k):
    return np.array([(k >> 16) & 255, (k >> 8) & 255, k & 255])


def nearest(k, among):
    return min(among, key=lambda m: ((rgb(k) - rgb(m)) ** 2).sum())


def subject_of(a, bg):
    """The biggest 4-connected part of cells that are not the background."""
    m = a != bg
    seen = np.zeros_like(m)
    best = []
    for sj, si in zip(*np.nonzero(m)):
        if seen[sj, si]:
            continue
        part, q = [], deque([(sj, si)])
        seen[sj, si] = True
        while q:
            j, i = q.popleft()
            part.append((j, i))
            for y, x in ((j + 1, i), (j - 1, i), (j, i + 1), (j, i - 1)):
                if 0 <= y < m.shape[0] and 0 <= x < m.shape[1] and m[y, x] and not seen[y, x]:
                    seen[y, x] = True
                    q.append((y, x))
        if len(part) > len(best):
            best = part
    s = np.zeros_like(m)
    for j, i in best:
        s[j, i] = True
    return s


def trail_of(Bd):
    """The trail of the subject (frames × rows × columns, 0 where it is not): (the period its leftmost columns repeat
    with, how many columns from the left edge the trail is) — (0, 0): none. Columns c and c + P are the same (but for up
    to 2 cells: a sparkle crossing it) from the edge on in every frame; the longest such run wins. Its length: from the
    edge while a column's top stays at the levels of its first period (a wavy trail: its wave, up to the character)."""
    F, ny, nx = Bd.shape
    if not Bd[:, :, 0].any():
        return 0, 0
    best = (0, 0)
    for P in range(4, nx // 2):
        run = nx
        for f in range(F):
            r = 0
            while r + P < nx and (Bd[f, :, r] != Bd[f, :, r + P]).sum() <= 2:
                r += 1
            run = min(run, r)
        if run >= 2 and run + P > best[1]:
            best = (P, run + P)
    P = best[0]
    if not P:
        return 0, 0
    def top(f, c):
        rows = np.nonzero(Bd[f, :, c])[0]
        return int(rows.min()) if len(rows) else -1
    length = nx
    for f in range(F):
        levels = {top(f, c) for c in range(P)}
        c = 0
        while c < nx and top(f, c) in levels:
            c += 1
        length = min(length, c)
    return P, length


def main():
    argv = sys.argv[1:]
    preview = None
    exact = "--exact" in argv
    argv = [a for a in argv if a != "--exact"]
    cell = None
    if "--cell" in argv:
        k = argv.index("--cell")
        cell = float(argv[k + 1])
        del argv[k:k + 2]
    if "--preview" in argv:
        k = argv.index("--preview")
        preview = argv[k + 1] if k + 1 < len(argv) else None
        del argv[k:k + 2]
    if len(argv) != 3 or not preview and "--preview" in sys.argv:
        sys.exit(__doc__)
    src, out, name = argv
    frames, ms = frames_of(src)
    if cell:
        px = py = cell
        ox = oy = 0.0
        nx, ny = int(round(frames[0].shape[1] / cell)), int(round(frames[0].shape[0] / cell))
    else:
        px, ox, nx = fit_axis(frames, 1)
        py, oy, ny = fit_axis(frames, 0)
    C = np.array([cells_of(f, px, ox, nx, py, oy, ny) for f in frames])
    print(f"{src}: {len(frames)} frames of {frames[0].shape[1]} × {frames[0].shape[0]} px; "
          f"a cell {px:.3f} × {py:.3f} px → {nx} × {ny} cells")

    # The colors: real ones — those of at least 5 % as many cells as the commonest (after the background); the rest are
    # the blur's leftovers (in a scaled-up GIF: 47 cells and fewer against 94 and more): one that is (nearly) a blend of two
    # real ones goes to the nearer of the two, any other — to the nearest real one
    count = Counter(C.ravel().tolist())
    bg = count.most_common(1)[0][0]
    top = max([n for k, n in count.items() if k != bg] or [1])
    real = [k for k, n in count.items() if exact or k == bg or n >= top * 0.05]
    remap = {k: k for k in real}
    for k in count:
        if k in remap:
            continue
        c, best = rgb(k), None
        for a in real:
            for b in real:
                A, B = rgb(a), rgb(b)
                t = float(np.clip(((c - A) @ (B - A)) / max(1, (B - A) @ (B - A)), 0, 1))
                d = ((A + t * (B - A) - c) ** 2).sum()
                if best is None or d < best[0]:
                    best = (d, a if t < 0.5 else b)
        remap[k] = best[1]
    C = np.vectorize(remap.get)(C)

    # The subject of every frame; around it — apart (sparkles). A sparkle crossing the subject: a cell of a color that
    # is apart somewhere, none of whose 8 neighbours has its color or the subject's darkest color (an outline)
    S = np.array([subject_of(a, bg) for a in C])
    apart_colors = sorted({k for a, s in zip(C, S) for k in a[(~s) & (a != bg)].ravel().tolist()})
    body_colors = sorted({k for a, s in zip(C, S) for k in a[s].ravel().tolist()})
    dark = min(body_colors, key=lambda k: rgb(k).sum()) if body_colors else None
    for a, s in zip(C, S):
        for j, i in zip(*np.nonzero(s)):
            k = a[j, i]
            if k not in apart_colors:
                continue
            near = a[max(0, j - 1):j + 2, max(0, i - 1):i + 2]
            if not ((near == k).sum() > 1 or (near == dark).any()):
                s[j, i] = False
    body_colors = sorted({k for a, s in zip(C, S) for k in a[s].ravel().tolist()})
    if len(body_colors) + len(apart_colors) > 15:
        sys.exit(f"too many colors: {len(body_colors)} + {len(apart_colors)} apart (up to 15)")
    palette = [0] + body_colors + apart_colors
    index_body = {k: 1 + n for n, k in enumerate(body_colors)}
    index_apart = {k: 1 + len(body_colors) + n for n, k in enumerate(apart_colors)}
    I = np.zeros(C.shape, np.uint8)
    for f in range(len(C)):
        for j in range(ny):
            for i in range(nx):
                k = C[f, j, i]
                if k != bg:
                    I[f, j, i] = index_body[k] if S[f, j, i] else index_apart[k]
    apart_bits = sum(1 << index_apart[k] for k in apart_colors)
    ys, xs = np.nonzero(S.any(axis=0))
    body = (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)
    ax, ay = (body[0] + body[2]) / 2, (body[1] + body[3]) / 2

    trail_p, trail_n = trail_of(np.where(S, I, 0))

    # The sparkles of each frame: their cells, by row then column (i | j << 10 | color << 20)
    sparks, spark_at = [], [0]
    for f in range(len(I)):
        for j in range(ny):
            for i in range(nx):
                k = int(I[f, j, i])
                if k and not S[f, j, i]:
                    sparks.append(i | j << 10 | k << 20)
        spark_at.append(len(sparks))

    # 4 bits a cell, the left cell in the high bits
    rb = (nx + 1) // 2
    packed = bytearray()
    for f in range(len(I)):
        for j in range(ny):
            row = list(I[f, j]) + [0]
            packed += bytes((row[2 * b] << 4) | row[2 * b + 1] for b in range(rb))
    frame_ms = int(round(float(np.median(ms))))
    changed = [int((I[f] != I[(f + 1) % len(I)]).sum()) for f in range(len(I))]
    print(f"  colors: background #{bg:06X} → nothing; {len(body_colors)} of the subject, {len(apart_colors)} apart "
          f"(hide bits 0x{apart_bits:04X}); the subject {body[2] - body[0]} × {body[3] - body[1]} cells")
    print(f"  {frame_ms} ms a frame; cells changed frame to frame: {min(changed)}..{max(changed)} of {nx * ny}; "
          f"{len(packed)} bytes")
    print("  the trail: " + (f"{trail_n} columns from the left edge, repeating every {trail_p}" if trail_p else "none"))
    print(f"  sparkles: {min(len(a) for a in [range(spark_at[f], spark_at[f + 1]) for f in range(len(I))])}.."
          f"{max(spark_at[f + 1] - spark_at[f] for f in range(len(I)))} cells a frame ({4 * len(sparks)} bytes)")

    up = name.upper()
    h = [f"// Generated by tools/halo_pixels.py from {src.split('/')[-1]} — do not edit by hand.",
         f"// {len(I)} frames of {nx} × {ny} cells, {frame_ms} ms each (lib/Halo: HaloPixels, HI_PIXELS); colors 1..{len(body_colors)}"
         f" — the subject, {len(body_colors) + 1}..{len(palette) - 1} — what is around it ({up}_APART hides it)",
         "#pragma once", "",
         f"static const uint32_t {name}_palette[16] = {{" +
         ", ".join(f"0x{k:06X}" for k in palette + [0] * (16 - len(palette))) + "};",
         f"static const uint8_t {name}_cells[{len(packed)}] = {{"]
    for b in range(0, len(packed), 24):
        h.append("    " + ", ".join(f"0x{v:02X}" for v in packed[b:b + 24]) + ",")
    h += ["};",
          f"static const uint32_t {name}_sparks[{max(1, len(sparks))}] = {{" + ", ".join(f"0x{v:X}" for v in sparks or [0]) + "};",
          f"static const uint16_t {name}_spark_at[{len(spark_at)}] = {{" + ", ".join(map(str, spark_at)) + "};",
          f"static const HaloPixels {name} = {{{nx}, {ny}, {len(I)}, {frame_ms}, {ax:.1f}f, {ay:.1f}f, {name}_palette, {name}_cells, "
          f"0x{apart_bits:04X}, {trail_p}, {trail_n}, {body[0]}, {body[1]}, {body[2]}, {body[3]}, {name}_sparks, {name}_spark_at}};",
          f"#define {up}_APART 0x{apart_bits:04X}  // the colors of what is around the subject (HaloItem.hide)",
          f"static const float {name}_body[4] = {{{body[0]}, {body[1]}, {body[2]}, {body[3]}}};  // the subject's cells: x0, y0, x1, y1",
          ""]
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("\n".join(h))
    print(f"  → {out}")

    if preview:
        pal = np.array([rgb(k) for k in palette] + [[0, 0, 0]] * (16 - len(palette)), np.uint8)
        pal[0] = (0, 0, 0)
        cols = min(len(I), 6)
        k = 4
        sheet = Image.new("RGB", (cols * (nx * k + 4), ((len(I) + cols - 1) // cols) * (ny * k + 4)), (40, 40, 44))
        for f in range(len(I)):
            im = Image.fromarray(pal[I[f]]).resize((nx * k, ny * k), Image.NEAREST)
            sheet.paste(im, ((f % cols) * (nx * k + 4), (f // cols) * (ny * k + 4)))
        sheet.save(preview)
        print(f"  preview → {preview}")


if __name__ == "__main__":
    main()
