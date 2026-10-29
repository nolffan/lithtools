#!/usr/bin/env python3
"""The strings each function of a shipped image refers to, in address order.

    build/strmap.py <target> [--all]

<target> is a label or stem of progress/targets.py. For every function of
its function table (out/reference/<stem>-functions.csv) inside the target's
app range, the 32-bit immediates and displacements of its instructions that
point at a NUL-terminated printable string in .rdata, .data or .data1
(Intel C/C++'s literals: NOLF's processor) are listed
with the string. Functions without one are left out unless --all.

A reading aid for tools/guides/13-cluster-recon.md step 1: literal order
follows function order in these images, so the strings name the modules.
"""
import csv
import os
import re
import struct
import subprocess
import sys

TALON = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(TALON, 'progress'))
import targets                                   # noqa: E402


def sections(d):
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    base = struct.unpack_from('<I', d, pe + 52)[0]
    ns = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    out = []
    for i in range(ns):
        o = pe + 24 + opt + 40 * i
        name = d[o:o + 8].rstrip(b'\0').decode()
        vsz, va, rsz, ptr = struct.unpack_from('<IIII', d, o + 8)
        out.append((name, base + va, max(vsz, rsz), ptr, rsz))
    return base, out


def string_at(d, secs, a):
    for name, va, sz, ptr, rsz in secs:
        if name in ('.rdata', '.data', '.data1') and va <= a < va + rsz:
            o = ptr + a - va
            e = d.find(b'\0', o, o + 400)
            if e < 0 or e - o < 2:
                return None
            s = d[o:e]
            if all(32 <= c < 127 or c in (9, 10, 13) for c in s):
                return s.decode('latin-1')
    return None


def main():
    want = sys.argv[1].lower()
    t = next(x for x in targets.TARGETS
             if want in (x['label'].lower(), x['stem'].lower()))
    img = os.path.join(TALON, t['shipped'])
    d = open(img, 'rb').read()
    base, secs = sections(d)
    funcs = []
    with open(os.path.join(TALON, targets.REFERENCE, t['stem'] + '-functions.csv')) as f:
        for row in csv.DictReader(f):
            a = int(row.get('addr') or row.get('address') or list(row.values())[0], 16)
            size = int(row.get('size') or list(row.values())[1])
            if any(lo <= a < hi for lo, hi in t['app']):
                funcs.append((a, size))
    funcs.sort()
    lo = min(a for a, _ in funcs)
    hi = max(a + s for a, s in funcs)
    dis = subprocess.run(['objdump', '-d', '--no-show-raw-insn', '-M', 'intel',
                          f'--start-address={lo:#x}', f'--stop-address={hi:#x}', img],
                         capture_output=True, text=True).stdout
    refs = {}
    for line in dis.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s*(.*)', line)
        if not m:
            continue
        a = int(m.group(1), 16)
        for v in re.findall(r'0x([0-9a-f]{6,8})\b', m.group(2)):
            s = string_at(d, secs, int(v, 16))
            if s is not None:
                refs.setdefault(a, []).append(s)
    show_all = '--all' in sys.argv
    import bisect
    starts = [a for a, _ in funcs]
    per = {}
    for a, ss in refs.items():
        i = bisect.bisect_right(starts, a) - 1
        if i >= 0:
            per.setdefault(starts[i], []).extend(ss)
    for a, size in funcs:
        ss = per.get(a, [])
        if ss or show_all:
            uniq = list(dict.fromkeys(ss))
            print(f'{a:#010x} {size:6}  ' + ' | '.join(repr(s)[1:-1][:70] for s in uniq))


if __name__ == '__main__':
    main()
