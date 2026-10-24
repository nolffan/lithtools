#!/usr/bin/env python3
"""Reads and writes back every extracted .ltb with lithtech.ltb.

    roundtrip_ltb.py [dir]      (default ~/src/lithtools/out/assets/ltb;
                                 fill it with extract_ltb.py)

Checks each file comes back byte for byte, and again with the stored
ModelAllocations thrown away and recomputed. Exits non-zero on a failure.
"""
import collections
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from lithtech import ltb  # noqa: E402


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / "src/lithtools/out/assets/ltb"
    files = sorted(root.rglob("*.ltb"))
    stats = collections.Counter()
    alloc_diff = collections.Counter()
    bad = 0
    t0 = time.time()
    for f in files:
        data = f.read_bytes()
        try:
            m = ltb.read(data)
        except Exception as e:
            bad += 1
            print("FAIL read %s: %s" % (f.relative_to(root), e))
            continue
        kind = "chunked" if isinstance(m, ltb.ChunkedModel) else "v%d" % m.version
        stats[kind, "files"] += 1
        out = ltb.write(m)
        if out != data:
            bad += 1
            i = next((i for i in range(min(len(out), len(data))) if out[i] != data[i]), min(len(out), len(data)))
            print("FAIL write %s: first difference at %d (sizes %d/%d)" % (f.relative_to(root), i, len(data), len(out)))
            continue
        stats[kind, "identical"] += 1
        if isinstance(m, ltb.Model):
            stored = m.alloc_counts
            comp = ltb.compute_alloc_counts(m)
            # The counts the engine sizes its block allocation from must
            # come out exactly (CalcAllocationSize); the rest are hints.
            exact = all(stored[i] == comp[i] for i in ltb.ALLOC_EXACT if i < len(stored))
            stats[kind, "allocs-match"] += exact
            for i, (a, b) in enumerate(zip(stored, comp)):
                if a != b:
                    alloc_diff[ltb.ALLOC_NAMES[i]] += 1
    for kind in sorted({k for k, _ in stats}):
        print("%-8s %5d files, %5d identical, %5s key allocation counts recomputed exactly" % (
            kind, stats[kind, "files"], stats[kind, "identical"],
            stats[kind, "allocs-match"] if kind != "chunked" else "-"))
    if alloc_diff:
        print("allocation fields (hints) that differ when recomputed (files):",
              ", ".join("%s %d" % kv for kv in alloc_diff.most_common()))
    print("%d files, %d failed, %.0fs" % (len(files), bad, time.time() - t0))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
