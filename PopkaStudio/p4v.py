#!/usr/bin/env python3
"""p4v.py — turn a video into a clip for the ESP32-P4 board's Video app: p4v.py SOURCE OUT.p4v [--q 3] [--mbit N] [--start S] [--seconds N]

The picture is MJPEG, 1024 x 576 (a whole number of the decoder's 16-pixel blocks; anything else in shape gets black bars),
the sound 16-bit mono at 32 kHz. The file (little-endian, docs/p4-video.md):

    64-byte header: "P4V2", codec 0, width, height, fps numerator, denominator, frame count, total size, sound rate,
                    the small picture's offset and size
    count + 1 offsets of the records, count JPEG sizes
    a record a frame, from a 512-byte boundary: the JPEG padded to 512, then its sound padded to 512
    the small picture: a 320 x 240 JPEG, what the tile on the screen shows

Needs ffmpeg and ffprobe.
"""
import argparse
import json
import math
import os
import struct
import subprocess
import tempfile

W, H, ALIGN = 1024, 576, 512
RATE = 32000
TIME_LIMIT = 3.9 * 2**30  # FAT32: a file under 4 GB
FPS = [(24000, 1001), (24, 1), (25, 1), (30000, 1001), (30, 1), (50, 1), (60000, 1001), (60, 1)]


def probe(path):
    out = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "stream=codec_type,avg_frame_rate,r_frame_rate:format=duration",
                          "-of", "json", path], check=True, capture_output=True, text=True).stdout
    return json.loads(out)


def pick_fps(streams):
    """The source's rate when it is a usual one; a phone's variable rate comes out as 30 (or 60)."""
    v = next((s for s in streams if s["codec_type"] == "video"), None)
    if not v:
        raise RuntimeError("no picture in this file")
    num, den = map(int, (v.get("avg_frame_rate") or v["r_frame_rate"]).split("/"))
    if not num or not den:
        num, den = map(int, v["r_frame_rate"].split("/"))
    fps = num / den
    best = min(FPS, key=lambda f: abs(f[0] / f[1] - fps))
    if abs(best[0] / best[1] - fps) / fps < 0.03:
        return best
    return (60, 1) if fps > 40 else (30, 1)


# The P4's decoder takes Y as TV range (16-235) and BT.601: an ordinary full-range JPEG comes out too contrasty
COLOR = "flags=lanczos:out_color_matrix=bt601:out_range=limited"


def poster(source, start, length):
    """The small picture: of five moments the one with most in it (a fade or a dark frame compresses to almost nothing)."""
    vf = f"scale=320:240:force_original_aspect_ratio=increase:{COLOR},crop=320:240,format=yuv420p"
    best = b""
    for k in (0.1, 0.25, 0.4, 0.55, 0.7) if length > 0 else (0,):
        r = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-ss", str(start + length * k), "-i", source, "-frames:v", "1", "-vf", vf, "-c:v", "mjpeg",
                            "-strict", "unofficial", "-q:v", "3", "-f", "image2pipe", "-"], capture_output=True)
        if r.returncode == 0 and r.stdout[:2] == b"\xff\xd8" and len(r.stdout) > len(best):
            best = r.stdout
    return best


def jpegs(stream):
    """The frames of ffmpeg's MJPEG stream (the end marker cannot occur inside a frame's data: 0xFF is stuffed there)."""
    buf = b""
    while chunk := stream.read(1 << 20):
        buf += chunk
        start = 0
        while (end := buf.find(b"\xff\xd9", start)) >= 0:
            yield buf[start:end + 2]
            start = end + 2
        buf = buf[start:]


def convert(source, out, quality=3, start=0, seconds=0, progress=None, mbit=0):
    """Writes out; progress(0..1) as it goes. Returns the clip's frames, fps and seconds. mbit — Mbit/s: instead of one
    quality for every frame, each frame's own so that a second never takes more (a clip for the network: a stream wants it even)"""
    info = probe(source)
    streams = info.get("streams", [])
    num, den = pick_fps(streams)
    fps = num / den
    length = float(info.get("format", {}).get("duration") or 0) - start
    if seconds:
        length = min(length, seconds) if length > 0 else seconds
    has_sound = any(s["codec_type"] == "audio" for s in streams)
    rate = RATE if has_sound else 0
    limit = int(math.ceil(length * fps)) + int(fps * 2) + 16 if length > 0 else int(fps * 3600 * 4)
    table = 4 * (2 * limit + 1)
    at = (64 + table + ALIGN - 1) & ~(ALIGN - 1)
    time_args = (["-ss", str(start)] if start else []) + ["-i", source] + (["-t", str(seconds)] if seconds else [])
    with tempfile.TemporaryDirectory() as tmp:
        pcm_path = os.path.join(tmp, "s.pcm")
        if has_sound:
            subprocess.run(["ffmpeg", "-nostdin", "-v", "error", *time_args, "-vn", "-ac", "1", "-ar", str(RATE), "-f", "s16le", pcm_path, "-y"], check=True)
        pcm = open(pcm_path, "rb") if has_sound else None
        vf = (f"fps={num}/{den},scale={W}:{H}:force_original_aspect_ratio=decrease:{COLOR},"
              f"pad={W}:{H}:(ow-iw)/2:(oh-ih)/2:color=black,format=yuv420p")
        cmd = ["ffmpeg", "-nostdin", "-v", "error", *time_args, "-an", "-vf", vf, "-c:v", "mjpeg", "-strict", "unofficial",
               *(["-b:v", f"{mbit:g}M", "-maxrate", f"{mbit:g}M", "-bufsize", f"{mbit / 4:g}M"] if mbit else ["-q:v", str(quality)]),
               "-f", "image2pipe", "-"]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        offsets, sizes = [], []
        size_at = at
        tmp_out = out + ".part"
        try:
            with open(tmp_out, "wb") as f:
                f.write(b"\0" * at)
                for i, jpeg in enumerate(jpegs(proc.stdout)):
                    if i >= limit:
                        break
                    offsets.append(size_at)
                    sizes.append(len(jpeg))
                    rec = jpeg + b"\0" * (-len(jpeg) % ALIGN)
                    if pcm:
                        first, last = (rate * den * i // num) * 2, (rate * den * (i + 1) // num) * 2
                        pcm.seek(first)
                        sound = pcm.read(last - first).ljust(last - first, b"\0")
                        rec += sound + b"\0" * (-len(sound) % ALIGN)
                    f.write(rec)
                    size_at += len(rec)
                    if size_at > TIME_LIMIT:
                        raise RuntimeError("the clip would be over 4 GB: too long for the card's file system")
                    if progress and i % 8 == 0 and length > 0:
                        progress(min(0.99, i / (length * fps)))
                if proc.wait():
                    raise RuntimeError("ffmpeg could not read this video")
                if not offsets:
                    raise RuntimeError("no frames in this video")
                offsets.append(size_at)
                small = poster(source, start, length)
                small_at = size_at
                f.write(small + b"\0" * (-len(small) % ALIGN))
                total = (small_at + len(small) + 4095) & ~4095
                f.truncate(total)
                head = struct.pack("<4s10I", b"P4V2", 0, W, H, num, den, len(sizes), total, rate, small_at if small else 0, len(small)).ljust(64, b"\0")
                f.seek(0)
                f.write(head)
                f.write(struct.pack(f"<{len(offsets)}I", *offsets))
                f.write(struct.pack(f"<{len(sizes)}I", *sizes))
            os.replace(tmp_out, out)
        finally:
            if proc.poll() is None:
                proc.kill()
            if pcm:
                pcm.close()
            if os.path.exists(tmp_out):
                os.remove(tmp_out)
    return {"frames": len(sizes), "fps": fps, "seconds": len(sizes) / fps, "size": os.path.getsize(out), "sound": has_sound}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("source")
    p.add_argument("out")
    p.add_argument("--q", type=int, default=3)
    p.add_argument("--mbit", type=float, default=0)
    p.add_argument("--start", type=float, default=0)
    p.add_argument("--seconds", type=float, default=0)
    a = p.parse_args()
    r = convert(a.source, a.out, a.q, a.start, a.seconds, lambda f: print(f"\r{f * 100:5.1f}%", end="", flush=True), a.mbit)
    print(f"\r{a.out}: {r['frames']} frames, {r['fps']:.2f} fps, {r['seconds']:.1f} s, {r['size'] / 1048576:.1f} MB, sound {r['sound']}")


if __name__ == "__main__":
    main()
