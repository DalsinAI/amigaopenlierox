#!/usr/bin/env python3
"""png_rgba.py DIR..: rewrites, in place, every 8-bit RGB PNG (colour type
2) in DIR as an RGBA PNG (colour type 6) with the same pixels, opaque
(a tRNS colour, if there is one, becomes alpha 0). The ancillary chunks
stay (sBIT gets an alpha entry).

Why: OpenGPU's SDL2_image.module (0.6) decodes PNGs on the host, and
decoding 8-bit RGB pictures whose rows are not a multiple of 4 bytes
(Starfighter's gfx/missileBoat1.png, 30 wide: 90 bytes a row) damages the
host's heap: with two or three of them loaded and kept, the emulator
aborts (glibc "corrupted size vs. prev_size"). RGBA rows are always a
multiple of 4, and RGBA pictures of the same sizes load cleanly. See
amiga/tests/rgb_filter.c and AMIGA_PORT.md. Run by amiga/Makefile.amigaos
on the drawer's copy of gfx/.
The AmiCradle Team. GPL 3 or later, like Starfighter."""
import os
import struct
import sys
import zlib

SIG = b"\x89PNG\r\n\x1a\n"


def chunks(data):
    i = 8
    while i < len(data):
        n, = struct.unpack(">I", data[i:i + 4])
        yield data[i + 4:i + 8], data[i + 8:i + 8 + n]
        i += 12 + n


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else b if pb <= pc else c


def predict(f, a, b, c):
    return [0, a, b, (a + b) // 2, paeth(a, b, c)][f]


def unfilter(raw, w, h, bpp):
    stride = w * bpp
    rows, prev = [], bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            c = prev[i - bpp] if i >= bpp else 0
            line[i] = (line[i] + predict(f, a, prev[i], c)) & 255
        rows.append(line)
        prev = line
    return rows


def refilter(rows, bpp):
    """each row with whichever filter gives the smallest sum (libpng's way)"""
    out, prev = bytearray(), bytearray(len(rows[0]))
    for line in rows:
        best = None
        for f in range(5):
            enc = bytearray(len(line))
            for i, v in enumerate(line):
                a = line[i - bpp] if i >= bpp else 0
                c = prev[i - bpp] if i >= bpp else 0
                enc[i] = (v - predict(f, a, prev[i], c)) & 255
            cost = sum(x if x < 128 else 256 - x for x in enc)
            if best is None or cost < best[0]:
                best = (cost, f, enc)
        out.append(best[1])
        out += best[2]
        prev = line
    return bytes(out)


def fix(path):
    data = open(path, "rb").read()
    if data[:8] != SIG:
        return False
    cl = list(chunks(data))
    w, h, depth, ctype, comp, filt, interlace = struct.unpack(">IIBBBBB", cl[0][1])
    if ctype != 2 or depth != 8 or interlace:
        return False
    rows = unfilter(zlib.decompress(b"".join(d for t, d in cl if t == b"IDAT")), w, h, 3)
    key = None
    for t, d in cl:
        if t == b"tRNS" and len(d) == 6:
            key = bytes(v & 255 for v in struct.unpack(">HHH", d))
    rgba = []
    for line in rows:
        out = bytearray()
        for x in range(w):
            px = bytes(line[x * 3:x * 3 + 3])
            out += px + (b"\0" if px == key else b"\xff")
        rgba.append(out)
    idat = zlib.compress(refilter(rgba, 4), 9)
    out, done = bytearray(SIG), False
    for t, d in cl:
        if t == b"IHDR":
            d = struct.pack(">IIBBBBB", w, h, 8, 6, comp, filt, 0)
        elif t == b"IDAT":
            if done:
                continue
            d, done = idat, True
        elif t == b"tRNS":
            continue
        elif t == b"sBIT" and len(d) == 3:
            d += b"\x08"
        out += struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    # check: the new file holds the same pixels
    ncl = list(chunks(bytes(out)))
    assert unfilter(zlib.decompress(b"".join(d for t, d in ncl if t == b"IDAT")), w, h, 4) == rgba, path
    open(path, "wb").write(out)
    return True


if __name__ == "__main__":
    for top in sys.argv[1:]:
        for name in sorted(os.listdir(top)):
            if name.lower().endswith(".png") and fix(os.path.join(top, name)):
                print("png_rgba: " + os.path.join(top, name))
