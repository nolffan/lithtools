#!/usr/bin/env python3
"""Check lithtech::pcx against ImageMagick, image by image.

    pcx_decode_all --dump <dir> <file.rez | file.pcx | folder>...
    compare_pcx.py <dir>

For each image pcx_decode_all dumped (<dir>/index.tsv), decodes the same
file with ImageMagick (`magick`, or `convert`, to 8-bit RGBA) and compares
the pixels; prints the ones that differ and a total. Exits non-zero if any
does. Needs ImageMagick (the lithrez docker image and the build image have
it).
"""
import shutil
import subprocess
import sys


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    d = sys.argv[1]
    im = shutil.which("magick") or shutil.which("convert")
    if not im:
        sys.exit("compare_pcx.py: ImageMagick (magick or convert) is needed")
    same = diff = 0
    for line in open(f"{d}/index.tsv"):
        n, w, h, src, name = line.rstrip("\n").split("\t")
        ours = open(f"{d}/{n}.rgba", "rb").read()
        theirs = subprocess.run([im, f"pcx:{src}", "-depth", "8", "rgba:-"], capture_output=True).stdout
        if ours == theirs:
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
