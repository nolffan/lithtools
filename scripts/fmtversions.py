#!/usr/bin/env python3
"""Tally the format versions of a game's worlds, models and textures.

    fmtversions.py <tools-dir> <file.rez | directory> [...]

<tools-dir> is the shared ~/src/tools (for rezextract.py). Every .rez given,
or found under a directory given, is walked; loose files under a directory
are read too. For each kind of file, the distinct header versions are
counted:

  .dat  world      uint32 version at 0 (Jupiter: CURRENT_WORLD_VERSION 85)
  .ltb  model      LTB_Header: uint8 file type at 0, uint16 version at 2;
                   then the model's uint32 file version at 20
  .abc  model      LithTech 2.x: "Header" chunk, its uint32 version
  .dtx  texture    int32 version at 4 (Jupiter: CURRENT_DTX_VERSION -5)
  .lta/.ltc        not read (text / compressed text)
"""
import collections
import os
import struct
import sys

sys.path.insert(0, sys.argv[1])
from rezextract import Rez                    # noqa: E402

KINDS = ('.dat', '.ltb', '.abc', '.dtx')


def version(ext, head):
    if len(head) < 24:
        return 'short'
    if ext == '.dat':
        return f'world v{struct.unpack_from("<I", head, 0)[0]}'
    if ext == '.ltb':
        typ, ver = head[0], struct.unpack_from('<H', head, 2)[0]
        mv = struct.unpack_from('<I', head, 20)[0]
        return f'ltb type {typ} v{ver}, model v{mv}'
    if ext == '.abc':
        n = struct.unpack_from('<H', head, 0)[0]
        if head[2:2 + n] == b'Header':
            return f'abc v{struct.unpack_from("<I", head, 2 + n + 4)[0]}'
        return 'abc ?'
    if ext == '.dtx':
        return f'dtx v{struct.unpack_from("<i", head, 4)[0]}'


def tally(label, items, out):
    for name, read in items:
        ext = os.path.splitext(name)[1].lower()
        if ext in KINDS:
            out[(label, version(ext, read()))] += 1


def rez_items(path):
    rez = Rez(path)
    for p, pos, size in rez.walk():
        def read(pos=pos, size=size):
            rez.f.seek(pos)
            return rez.f.read(min(size, 64))
        yield p, read


def main():
    out = collections.Counter()
    for arg in sys.argv[2:]:
        if os.path.isfile(arg):
            tally(os.path.basename(arg), rez_items(arg), out)
            continue
        for dp, _, fs in os.walk(arg):
            for f in fs:
                p = os.path.join(dp, f)
                if f.lower().endswith('.rez'):
                    try:
                        tally(os.path.relpath(p, arg), rez_items(p), out)
                    except RuntimeError:
                        pass
                elif os.path.splitext(f)[1].lower() in KINDS:
                    tally('(loose)', [(f, lambda p=p: open(p, 'rb').read(64))], out)
    for (label, ver), n in sorted(out.items()):
        print(f'{label:40} {ver:40} {n:6}')


if __name__ == '__main__':
    main()
