#!/usr/bin/env python3
"""Inventory the PE binaries of an unpacked tools release.

    inventory.py <tools-dir> <name>=<root> [...]    > inventory.tsv

<tools-dir> is the shared ~/src/tools (for richhdr.py). Each <root> is
walked for PE images (MZ + PE signature, any extension). InstallShield's own
support groups (_Engine_*, _Support_*) and the DirectX redistributable are
skipped. One row per image:

  item, path, size, sha256 (first 16), kind (exe/dll), machine,
  PE timestamp (UTC), linker version, C/C++ compiler (from the Rich header:
  the C++ entry with the most objects, else the C one), MFC/CRT DLLs
  imported, 3ds Max / Maya plugin, FileVersion, CodeView PDB path.
"""
import hashlib
import os
import struct
import sys
import time

sys.path.insert(0, sys.argv[1])
from richhdr import rich                      # noqa: E402

CXX = {0x0b: 'VC6 C++', 0x0a: 'VC6 C', 0x1d: 'VC7.0 C++', 0x1c: 'VC7.0 C',
       0x60: 'VC7.1 C++', 0x5f: 'VC7.1 C', 0x64: 'VC7.1 C++ LTCG',
       0x16: 'VC6 C++ (Std)', 0x62: 'VC7.1 C++ (Std)',
       # prodid 0x30/0x31: VC6 SP5 + Processor Pack (back end 13.00.9044),
       # toolchains/msvc-6.00-9044; checked by compiling with it
       0x30: 'VC6PP C', 0x31: 'VC6PP C++'}
SKIP = ('_Engine_', '_Support_', 'DirectX')


def sections(d, pe):
    ns = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    out = []
    for i in range(ns):
        o = pe + 24 + opt + 40 * i
        vsz, va, rsz, ptr = struct.unpack_from('<IIII', d, o + 8)
        out.append((va, max(vsz, rsz), ptr))
    return out


def rva2off(secs, rva):
    for va, sz, ptr in secs:
        if va <= rva < va + sz:
            return ptr + rva - va
    return None


def cstr(d, o):
    return d[o:d.index(b'\0', o)].decode('latin-1')


def imports(d, pe, secs):
    opt = pe + 24
    rva = struct.unpack_from('<I', d, opt + 104)[0]
    o = rva2off(secs, rva) if rva else None
    names = []
    while o is not None and o + 20 <= len(d):
        name = struct.unpack_from('<I', d, o + 12)[0]
        if not name:
            break
        no = rva2off(secs, name)
        if no is None:
            break
        names.append(cstr(d, no).lower())
        o += 20
    return names


def pdb(d, pe, secs):
    opt = pe + 24
    rva, size = struct.unpack_from('<II', d, opt + 96 + 6 * 8)
    o = rva2off(secs, rva) if rva else None
    if o is None:
        return ''
    for k in range(size // 28):
        typ, _, _, ptr = struct.unpack_from('<IIII', d, o + 28 * k + 12)
        if typ == 2 and ptr + 4 <= len(d):
            sig = d[ptr:ptr + 4]
            if sig == b'RSDS':
                return cstr(d, ptr + 24)
            if sig == b'NB10':
                return cstr(d, ptr + 16)
    return ''


def file_version(d):
    key = 'FileVersion'.encode('utf-16-le') + b'\0\0'
    i = d.find(key)
    if i < 0:
        return ''
    i += len(key)
    i += (-i) % 4
    j = i
    while j + 1 < len(d) and d[j:j + 2] != b'\0\0':
        j += 2
    return d[i:j].decode('utf-16-le', 'replace').strip()


def compiler(path):
    try:
        r = rich(path)
    except Exception:
        r = None
    if not r:
        return 'no Rich header'
    cands = [(cnt, CXX[pid], build) for pid, build, cnt in r if pid in CXX]
    if not cands:
        if any(pid == 0x00 and cnt for pid, _, cnt in r):
            return 'no @comp.id (Intel/pre-VC6)'
        return '?'
    cxx = [c for c in cands if 'C++' in c[1]] or cands
    cnt, name, build = max(cxx)
    return f'{name} {build} x{cnt}'


def scan(item, root):
    for dp, dn, fn in os.walk(root):
        dn[:] = sorted(x for x in dn if not x.startswith(SKIP))
        for f in sorted(fn):
            p = os.path.join(dp, f)
            with open(p, 'rb') as fh:
                head = fh.read(0x400)
            if head[:2] != b'MZ' or len(head) < 0x40:
                continue
            pe = struct.unpack_from('<I', head, 0x3c)[0]
            d = open(p, 'rb').read()
            if pe + 24 > len(d) or d[pe:pe + 4] != b'PE\0\0':
                continue
            mach, _, ts = struct.unpack_from('<HHI', d, pe + 4)
            chars = struct.unpack_from('<H', d, pe + 22)[0]
            magic, lmaj, lmin = struct.unpack_from('<HBB', d, pe + 24)
            if magic != 0x10b:
                continue
            secs = sections(d, pe)
            try:
                imps = imports(d, pe, secs)
            except Exception:
                imps = []
            rt = sorted({i for i in imps if i.startswith(('mfc', 'msvcr', 'msvcp'))})
            plug = ''
            if any(i in imps for i in ('core.dll', 'maxutil.dll', 'geom.dll')):
                plug = '3ds Max'
            elif any(i.startswith('openmaya') for i in imps):
                plug = 'Maya'
            try:
                pdbp = pdb(d, pe, secs)
            except Exception:
                pdbp = ''
            yield [item, os.path.relpath(p, root), str(len(d)),
                   hashlib.sha256(d).hexdigest()[:16],
                   'dll' if chars & 0x2000 else 'exe', hex(mach),
                   time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(ts)),
                   f'{lmaj}.{lmin:02d}', compiler(p), ' '.join(rt), plug,
                   file_version(d), pdbp]


def main():
    print('\t'.join(['item', 'path', 'size', 'sha256_16', 'kind', 'machine',
                     'pe_timestamp_utc', 'linker', 'compiler', 'runtime_dlls',
                     'plugin', 'file_version', 'pdb']))
    for arg in sys.argv[2:]:
        item, root = arg.split('=', 1)
        for row in scan(item, root):
            print('\t'.join(row))


if __name__ == '__main__':
    main()
