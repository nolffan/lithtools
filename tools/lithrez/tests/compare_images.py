#!/usr/bin/env python3
"""Check lithtech::pcx, ::tga and ::bmp against ImageMagick, image by image.

    image_decode_all --dump <dir> <file.rez | image | folder>...
    compare_images.py <dir>

For each image image_decode_all dumped (<dir>/index.tsv), decodes the same
file with ImageMagick (`magick`, or `convert`, to 8-bit RGBA) and compares
the pixels; prints the ones that differ and a total. Exits non-zero if any
does. One known disagreement is allowed: a 32-bit BI_RGB bitmap whose fourth
byte the format says is unused, which we read as opaque (as Windows draws
it) and ImageMagick as alpha; those count as identical when the colours
are. Needs ImageMagick (the lithrez docker image and the build image have
it).
"""
import shutil
import subprocess
import sys


def unused_alpha(path):
    """A 32-bit BI_RGB bitmap with a 40-byte header (no alpha mask)."""
    b = open(path, "rb").read(34)
    return len(b) == 34 and b[:2] == b"BM" and int.from_bytes(b[14:18], "little") == 40 and \
        int.from_bytes(b[28:30], "little") == 32 and int.from_bytes(b[30:34], "little") == 0


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    d = sys.argv[1]
    im = shutil.which("magick") or shutil.which("convert")
    if not im:
        sys.exit("compare_images.py: ImageMagick (magick or convert) is needed")
    same = diff = 0
    for line in open(f"{d}/index.tsv"):
        n, w, h, ext, src, name = line.rstrip("\n").split("\t")
        ours = open(f"{d}/{n}.rgba", "rb").read()
        theirs = subprocess.run([im, f"{ext}:{src}", "-depth", "8", "rgba:-"], capture_output=True).stdout
        if ours == theirs:
            same += 1
            continue
        if ext == "bmp" and len(ours) == len(theirs) and unused_alpha(src) and \
                all(ours[i] == theirs[i] for i in range(len(ours)) if i % 4 != 3):
            same += 1
            continue
        diff += 1
        if len(ours) != len(theirs):
            print(f"{name}: {len(ours)} bytes, ImageMagick {len(theirs)} ({w}x{h})")
            continue
        i = next(i for i in range(len(ours)) if ours[i] != theirs[i])
        px = i // 4
        print(f"{name}: first difference at pixel ({px % int(w)}, {px // int(w)}): "
              f"ours {ours[px * 4:px * 4 + 4].hex()}, ImageMagick {theirs[px * 4:px * 4 + 4].hex()}")
    print(f"{same} identical, {diff} different")
    sys.exit(1 if diff else 0)


if __name__ == "__main__":
    main()
