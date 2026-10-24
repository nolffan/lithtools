#!/usr/bin/env python3
"""Draws rezstudio.png (256x256): a crate in LithTech orange on dark
slate. Pure Python, so it needs nothing installed."""
import struct
import zlib

N = 256


def inside_round_rect(x, y, x0, y0, x1, y1, r):
    if x < x0 or x > x1 or y < y0 or y > y1:
        return False
    cx = min(max(x, x0 + r), x1 - r)
    cy = min(max(y, y0 + r), y1 - r)
    return (x - cx) ** 2 + (y - cy) ** 2 <= r * r


def pixel(x, y):
    if not inside_round_rect(x, y, 8, 8, 247, 247, 40):
        return (0, 0, 0, 0)
    t = y / N
    bg = (int(38 + 20 * t), int(44 + 18 * t), int(56 + 22 * t), 255)
    # the crate
    if 56 <= x <= 200 and 64 <= y <= 200:
        edge = x < 68 or x > 188 or y < 76 or y > 188
        diag = abs((x - 56) - (y - 64)) < 9 or abs((x - 56) - (200 - y)) < 9
        if edge or diag:
            return (250, 150, 40, 255)
        return (196, 104, 30, 255)
    # lid
    if 48 <= x <= 208 and 44 <= y <= 64:
        return (255, 176, 70, 255)
    return bg


rows = b"".join(b"\0" + b"".join(struct.pack("4B", *pixel(x, y)) for x in range(N)) for y in range(N))


def chunk(t, d):
    return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)


png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", N, N, 8, 6, 0, 0, 0)) + \
    chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b"")
open("rezstudio.png", "wb").write(png)
