#!/usr/bin/env python3
"""Reads and writes back every .rez given (or listed in a file with @list)
with `lithrez roundtrip` and compares the bytes.

    roundtrip_shipped.py <lithrez> <file.rez | @list>...

A file that differs only inside the gaps RezMgr leaves after each directory
block (zeros from a clean write, leftover disk bytes in some files written
on Windows 9x) counts as identical-but-for-gaps.
"""
import os
import struct
import subprocess
import sys
import tempfile


def gaps(x):
    ver, rp, rs = struct.unpack_from("<3I", x, 127)
    out = []

    def walk(pos, size):
        o, end = pos, pos + size
        subs = []
        while o < end:
            k, = struct.unpack_from("<I", x, o)
            o += 4
            if k == 1:
                dp, ds, dt = struct.unpack_from("<3I", x, o)
                o = x.index(b"\0", o + 12) + 1
                subs.append((dp, ds))
            else:
                nk, = struct.unpack_from("<I", x, o + 20)
                o = x.index(b"\0", o + 24) + 1
                o = x.index(b"\0", o) + 1 + 4 * nk
        out.append((pos + size, pos + 2 * size))
        for s in subs:
            if s[1]:
                walk(*s)

    walk(rp, rs)
    return out


def main():
    exe, args = sys.argv[1], sys.argv[2:]
    files = []
    for a in args:
        files += open(a[1:]).read().split("\n") if a.startswith("@") else [a]
    files = [f for f in files if f]
    same = gapped = bad = 0
    with tempfile.TemporaryDirectory() as td:
        out = os.path.join(td, "rt.rez")
        for f in files:
            r = subprocess.run([exe, "roundtrip", f, out], capture_output=True, text=True)
            x = open(f, "rb").read()
            y = open(out, "rb").read() if r.returncode == 0 else b""
            if x == y:
                same += 1
                continue
            if r.returncode == 0 and len(x) == len(y):
                z = bytearray(x)
                for a, b in gaps(x):
                    z[a:min(b, len(z))] = bytes(min(b, len(z)) - a)
                if bytes(z) == y:
                    gapped += 1
                    continue
            bad += 1
            i = next((i for i in range(min(len(x), len(y))) if x[i] != y[i]), min(len(x), len(y)))
            print("DIFF %s (sizes %d/%d, first at %d) %s" % (f, len(x), len(y), i, r.stderr.strip()))
    print("%d files: %d identical, %d identical but for leftover bytes in gaps, %d differ"
          % (len(files), same, gapped, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
