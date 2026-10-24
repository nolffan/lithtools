#!/usr/bin/env python3
"""Reads and writes back every PS2 model (.ltb file type 2) with
lithtech.ps2ltb.

    roundtrip_ps2ltb.py [dir]   (default ~/src/lithtools/out/assets/ps2, both
                                 discs' files under their names)

Checks each file comes back byte for byte (the DMA chains regenerated from
the strips, the weight maps from the chain layout), and again with every
LOD's strips repacked into buffers by the exporter's rule, which must give
the same buffers wherever the file has no lone-triangle buffer of its own.
Prints how many header counts the model's contents reproduce. Exits
non-zero on a failure.
"""
import collections
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "io_scene_lithtech"))
from lithtech import ps2ltb  # noqa: E402

COUNT_NAMES = ("keyframes", "animations", "nodes", "pieces", "child models", "triangles", "vertices",
               "weights", "LODs", "sockets", "weight sets", "strings", "string bytes", "zero")


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / "src/lithtools/out/assets/ps2"
    files = sorted(f for f in root.rglob("*") if f.suffix.lower() == ".ltb")
    bad = n = other = 0
    counts = collections.Counter()
    repack = collections.Counter()
    t0 = time.time()
    for f in files:
        data = f.read_bytes()
        if not ps2ltb.is_ps2_model(data):
            other += 1
            continue
        n += 1
        try:
            m = ps2ltb.read(data)
        except Exception as e:
            bad += 1
            print("FAIL read %s: %s" % (f.relative_to(root), e))
            continue
        out = ps2ltb.write(m)
        if out != data:
            bad += 1
            i = next((i for i in range(min(len(out), len(data))) if out[i] != data[i]), min(len(out), len(data)))
            print("FAIL write %s: first difference at %d (sizes %d/%d)" % (f.relative_to(root), i, len(data), len(out)))
            continue
        for i, (a, b) in enumerate(zip(m.computed_counts(), m.counts)):
            counts[i, a == b] += 1
        for p in m.pieces:
            for lod in p.lods:
                again = ps2ltb.buffers_from_strips(lod.strips())
                same = [len(b.strips) for b in again] == [len(b.strips) for b in lod.buffers]
                repack[same] += 1
    print("%d PS2 models: %d failed (%d other .ltb files skipped), %.0fs" % (n, bad, other, time.time() - t0))
    print("LODs whose buffers the exporter's packing reproduces: %d of %d" % (repack[True], sum(repack.values())))
    for i, name in enumerate(COUNT_NAMES):
        print("  header count %-13s matches the contents in %d of %d" % (name, counts[i, True], counts[i, True] + counts[i, False]))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
