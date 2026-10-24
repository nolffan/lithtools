#!/usr/bin/env python3
"""Round-trips the PS2 NOLF textures through lithtech.dtx.

The PS2 discs' files are extracted (gitignored) under
~/src/lithtools/out/assets/ps2/<disc>/ by the lithrez PS2 work; every .dtx
there is tested (twelve PS2 files named .dtx are really binary attribute
files, and are counted apart).

For each texture: read and write back byte for byte; decode every level
with stored and with PS2 palette alpha; for BPP_32P, map the PS2-decoded
image back onto its own palette (encode_32p with that palette) and count
the levels whose indices come back exactly (a palette with two equal
entries can't: either index is right). Then quantise a sample from
scratch and report how far the result is from the original.

    roundtrip_ps2_dtx.py [--hash OUT] [--sample N] [dirs...]

--hash writes "<path>\\t<format>\\t<fnv1a of the PS2-alpha RGBA of mip 0>"
per texture, as tools/lithrez's `dtx_decode_all --hash --ps2-alpha` does,
to compare the two decoders. Run it with Blender's Python (numpy) for
speed; plain Python gives the same answers.
"""
import argparse
import collections
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "io_scene_lithtech"))

from lithtech import dtx  # noqa: E402

ROOT = Path("~/src/lithtools/out/assets/ps2").expanduser()


def fnv(b):
    h = 1469598103934665603
    for x in b:
        h = ((h ^ x) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h




def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dirs", nargs="*")
    ap.add_argument("--hash")
    ap.add_argument("--sample", type=int, default=60)
    a = ap.parse_args()
    dirs = [Path(d) for d in a.dirs] or sorted(p for p in ROOT.iterdir() if p.is_dir())
    files = sorted(f for d in dirs for f in d.rglob("*") if f.suffix.lower() == ".dtx" and f.is_file())
    t0 = time.time()
    n = same = levels = 0
    not_dtx = 0
    remap_ok = remap_all = 0
    fmts = collections.Counter()
    fails = []
    hashes = []
    sample = []
    for f in files:
        d = f.read_bytes()
        if d[:4] != b"\0\0\0\0":
            not_dtx += 1
            continue
        n += 1
        try:
            t = dtx.read(d)
            same += dtx.write(t) == d
            fmts[t.bpp_name] += 1
            for m in range(t.mip_count):
                t.decode(m)
                w, h, rgba = t.decode(m, ps2_alpha=True)
                levels += 1
                if m == 0 and a.hash:
                    hashes.append("%s\t%s\t%016x" % (f, t.bpp_name, fnv(rgba)))
            if t.bpp == dtx.BPP_32P:
                pal = t.palette(ps2_alpha=True)
                w, h, rgba = t.decode(0, ps2_alpha=True)
                _, mips = dtx.encode_32p(w, h, rgba, 1, palette=pal)
                remap_all += 1
                # equal palette entries make either index right: compare colours
                remap_ok += mips[0] == t.mips[0] or dtx.decode_pixels(
                    dtx.BPP_32P, w, h, mips[0], pal) == rgba
                if len(sample) < a.sample and n % 37 == 0:
                    sample.append((f, w, h, rgba))
        except Exception as e:  # noqa: BLE001
            fails.append((f, e))
    print("%d PS2 textures (%d files named .dtx aren't textures): %d byte-identical, %d levels decoded, %d failed"
          % (n, not_dtx, same, levels, len(fails)))
    print("  formats:", dict(fmts))
    print("  BPP_32P mapped back onto their own palettes: %d of %d give the same image" % (remap_ok, remap_all))
    for f, e in fails[:10]:
        print("  FAIL", f, e)
    err = []
    for f, w, h, rgba in sample:
        t = dtx.from_rgba(w, h, rgba, bpp=dtx.BPP_32P, mipmaps=1)
        back = t.decode(0, ps2_alpha=True)[2]
        err.append(sum(abs(x - y) for x, y in zip(back, rgba)) / len(rgba))
        assert dtx.read(dtx.write(t)).decode(0, ps2_alpha=True)[2] == back
    if err:
        print("  quantised again from the image (%d textures): mean error %.2f, worst %.2f per channel"
              % (len(err), sum(err) / len(err), max(err)))
    if a.hash:
        Path(a.hash).write_text("\n".join(hashes) + "\n")
    print("  %.0fs" % (time.time() - t0))
    sys.exit(1 if fails or same != n else 0)


if __name__ == "__main__":
    main()
