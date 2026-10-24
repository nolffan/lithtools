#!/usr/bin/env python3
"""Recovers the file names of a PS2 LITH_PS2.REZ, which stores only their
hashes, and writes the list lithrez compiles in (core/data/ps2-names.txt).

    ps2_names.py --out core/data/ps2-names.txt --butes-out core/data/ps2-bute-names.txt
                 --pc <PC NOLF game dir>... <PS2 ISO or LITH_PS2.REZ>...

It also writes the names of the attribute tags and keys (--butes-out): the
PS2's compiled attribute files (.but) keep only their hashes (h * 31 + c,
case-sensitive), and every [Tag] and Key = in the PS2's text files and the
PC games' attribute files names all of them.

Candidates, tried against every disc's hashes (docs/PS2REZ.md):
  1. every path in the PC NOLF archives (*.rez in the --pc directories);
  2. every string in the PS2 files themselves and in the PS2 executable,
     split at quotes, spaces and the like;
  3. the folders of the names found so far, combined with every file name
     seen anywhere;
  4. identifiers (from the above and from each text file found) as
     worlds\\X.ltb, assetlists\\X.but/.txt, attributes\\X.but/.txt and
     lps\\X.lps, repeated while text files keep adding names.
The hash is weak (h = h * 11 + c), so related strings give related hashes and
some candidates match a file they don't name. A name is kept only when its
extension is one the games use and agrees with the file's contents (a .dtx
must be a DTX, and so on) and it has no stray characters; the first good name found for a hash wins
(candidates are tried in sorted order, so the list comes out the same each
time).
"""
import argparse
import os
import re
import struct
import subprocess
import sys

SECTOR = 2048


def rez_hash(name, weight=10):
    h = 0
    for ch in name:
        c = ord(ch.upper())
        if c == ord('/'):
            c = ord('\\')
        h = (h + (c & 0xFF) + h * weight) & 0xFFFFFFFF
    return h


def iso_files(path):
    """{NAME: (offset, size)} of an ISO9660 image's root directory, or None."""
    with open(path, 'rb') as f:
        f.seek(16 * SECTOR)
        pvd = f.read(SECTOR)
        if pvd[1:6] != b'CD001':
            return None
        root = pvd[156:190]
        ext, size = struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0]
        f.seek(ext * SECTOR)
        d = f.read(size)
    out, p = {}, 0
    while p < len(d):
        ln = d[p]
        if ln == 0:
            p = (p // SECTOR + 1) * SECTOR
            continue
        e, s, nl = struct.unpack_from('<I', d, p + 2)[0], struct.unpack_from('<I', d, p + 10)[0], d[p + 32]
        out[d[p + 33:p + 33 + nl].decode('latin-1').split(';')[0].upper()] = (e * SECTOR, s)
        p += ln
    return out


class Ps2Rez:
    def __init__(self, path):
        self.files = iso_files(path)
        self.f = open(path, 'rb')
        self.base = self.files['LITH_PS2.REZ'][0] if self.files else 0
        self.f.seek(self.base)
        _ver, n, self.weight, _data, _ntab = struct.unpack('<5I', self.f.read(20))
        self.f.seek(self.base + SECTOR)
        raw = self.f.read(n * 16)
        self.entries = {}
        for i in range(n):
            h, sec, size, _ = struct.unpack_from('<4I', raw, i * 16)
            self.entries.setdefault(h, (sec, size))   # zero-length files too

    def read(self, h, limit=None):
        sec, size = self.entries[h]
        self.f.seek(self.base + sec * SECTOR)
        return self.f.read(size if limit is None else min(size, limit))

    def exe(self):
        for n, (off, size) in (self.files or {}).items():
            if re.fullmatch(r'S[LC][UE]S_\d+\.\d+', n):
                self.f.seek(off)
                return self.f.read(size)
        return b''


def pc_paths(dirs, texts=None):
    """Every resource path in the .rez files of the given directories (and,
    into texts, the contents of their ATTRIBUTES\\*.TXT)."""
    out = set()

    def walk(f, pos, size, prefix):
        f.seek(pos)
        d = f.read(size)
        o, subs = 0, []
        while o + 4 <= len(d):
            (kind,) = struct.unpack_from('<I', d, o)
            o += 4
            if kind == 1:
                dpos, dsize, _ = struct.unpack_from('<3I', d, o)
                o += 12
                e = d.index(b'\0', o)
                subs.append((d[o:e].decode('latin-1'), dpos, dsize))
                o = e + 1
            else:
                fpos, fsize, _t, _i = struct.unpack_from('<4I', d, o)
                o += 16
                tag = d[o:o + 4]
                (nkeys,) = struct.unpack_from('<I', d, o + 4)
                o += 8
                e = d.index(b'\0', o)
                name = d[o:e].decode('latin-1')
                o = e + 2 + 4 * nkeys
                ext = tag.rstrip(b'\0')[::-1].decode('latin-1')
                out.add(prefix + name + ('.' + ext if ext else ''))
                if texts is not None and ext.upper() == 'TXT' and prefix.upper().startswith('ATTRIBUTES'):
                    here = f.tell()
                    f.seek(fpos)
                    texts.append(f.read(fsize).decode('latin-1'))
                    f.seek(here)
        for name, dpos, dsize in subs:
            walk(f, dpos, dsize, prefix + name + '\\')

    for d in dirs:
        for fn in sorted(os.listdir(d)):
            if fn.lower().endswith('.rez'):
                with open(os.path.join(d, fn), 'rb') as f:
                    hdr = f.read(168)
                    if b'RezMgr' not in hdr[:120]:
                        continue
                    _v, rpos, rsize = struct.unpack_from('<3I', hdr, 127)
                    try:
                        walk(f, rpos, rsize, '')
                    except (ValueError, struct.error):
                        pass
    return out


def kind(d):
    if d[4:8] == b'\xfb\xff\xff\xff':
        return 'dtx'
    if d[:4] == b'\x02\x00\x00\x00' or d[:4] == b'\x42\x00\x00\x00':
        return 'ltb'
    if d[:4] == b'Lith':
        return 'lps'
    if d[:4] == b'RIFF':
        return 'wav'
    if d[:2] == b'\x0a\x05':
        return 'pcx'
    return None


GOOD = re.compile(r'[A-Za-z0-9_\-&+]+(\\[A-Za-z0-9_\-&+ .]+)*\.[A-Za-z0-9]{1,4}')


EXTS = {'ltb', 'lps', 'but', 'ftb'}   # the PS2's own; the PC archives' are added in main()


def plausible(name, data):
    if not GOOD.fullmatch(name):
        return False
    ext = name.rsplit('.', 1)[-1].lower()
    if ext not in EXTS:
        return False
    if not data:
        return True   # a zero-length file: nothing to check against
    k = kind(data)
    if ext in ('dtx', 'ltb', 'lps', 'wav', 'pcx'):
        return k == ext
    if ext in ('but', 'txt', 'spr'):
        return k is None
    return True


# Names the text files don't have: the compiled Layout of the PS2 variant
# tables (an unnamed .but) was built from a source that spelled FlagItl
# "FlatItl", so the game never finds that key (it asks for FlagItl).
EXTRA_BUTE_NAMES = {'FlatItl'}


def bute_words(text):
    """The tag and key names in an attribute file's text."""
    return set(re.findall(r'\[([^\]\r\n]+)\]', text)) | set(re.findall(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=', text, re.M))


def recover(rez, pcp, found):
    head = {h: rez.read(h, 16) for h in rez.entries}

    def try_name(s):
        s = s.strip().replace('/', '\\').lstrip('\\')
        if not s:
            return False
        h = rez_hash(s, rez.weight)
        if h in head and h not in found and plausible(s, head[h]):
            found[h] = s
            return True
        return False

    for p in sorted(pcp):
        try_name(p)
    rx = re.compile(rb'[\x20-\x7e]{3,}')
    strs = set()
    for h in rez.entries:
        for m in rx.finditer(rez.read(h, 4 << 20)):
            strs.add(m.group().decode('latin-1'))
    for m in rx.finditer(rez.exe()):
        strs.add(m.group().decode('latin-1'))
    for s in sorted(strs):
        for part in re.split(r'["\s,;=()<>|%]+', s):
            if '.' in part or '\\' in part or '/' in part:
                try_name(part)
    dirs, bases = set(), set()
    for s in list(found.values()) + list(pcp):
        if '\\' in s:
            dirs.add(s.rsplit('\\', 1)[0])
        bases.add(s.rsplit('\\', 1)[-1])
    for s in strs:
        for part in re.split(r'["\s,;=()<>|\\/]+', s):
            if '.' in part and len(part) < 40:
                bases.add(part)
    for d in sorted(dirs):
        for b in sorted(bases):
            try_name(d + '\\' + b)
    idents = {s.rsplit('\\', 1)[-1].rsplit('.', 1)[0] for s in found.values()}
    for s in strs:
        idents.update(re.findall(r'[A-Za-z0-9_]{3,40}', s))
    pats = ['assetlists\\%s.but', 'assetlists\\%s.txt', 'attributes\\%s.but', 'attributes\\%s.txt',
            'worlds\\%s.ltb', 'lps\\%s.lps']
    while True:
        n0 = len(found)
        for i in sorted(idents):
            for p in pats:
                try_name(p % i)
        for h, s in sorted(found.items()):
            if s.lower().endswith('.txt'):
                t = rez.read(h).decode('latin-1')
                for q in re.findall(r'"([^"]+)"', t):
                    try_name(q)
                    idents.add(q.replace('/', '\\').rsplit('\\', 1)[-1].rsplit('.', 1)[0])
                idents.update(re.findall(r'[A-Za-z0-9_]{3,40}', t))
        if len(found) == n0:
            break
    return len(found)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--butes-out')
    ap.add_argument('--pc', action='append', default=[])
    ap.add_argument('discs', nargs='+')
    a = ap.parse_args()
    pc_texts = []
    pcp = pc_paths(a.pc, pc_texts)
    EXTS.update(p.rsplit('.', 1)[-1].lower() for p in pcp if '.' in p)
    print('%d PC paths' % len(pcp))
    names = {}
    for disc in a.discs:
        rez = Ps2Rez(disc)
        found = {h: n for h, n in names.items() if h in rez.entries and plausible(n, rez.read(h, 16))}
        got = recover(rez, pcp, found)
        print('%s: %d of %d named' % (os.path.basename(disc), got, len(rez.entries)))
        for h, n in found.items():
            names.setdefault(h, n)
        for h, n in found.items():
            if n.lower().endswith('.txt'):
                pc_texts.append(rez.read(h).decode('latin-1'))
    with open(a.out, 'w', newline='\n') as f:
        f.write('# PS2 LITH_PS2.REZ file names, recovered by tests/ps2_names.py from the US and\n'
                '# European discs (the archive stores only hashes). One name per line.\n')
        for h in sorted(names, key=lambda h: names[h].lower()):
            f.write(names[h] + '\n')
    print('%d names written' % len(names))
    if a.butes_out:
        words = set(EXTRA_BUTE_NAMES)
        for t in pc_texts:
            words |= bute_words(t)
        with open(a.butes_out, 'w', newline='\n') as f:
            f.write('# Attribute tag and key names for the PS2\'s compiled attribute files (.but),\n'
                    '# collected by tests/ps2_names.py from the PS2 and PC text attribute files.\n')
            for w in sorted(words, key=lambda w: (w.lower(), w)):
                f.write(w + '\n')
        print('%d attribute names written' % len(words))


if __name__ == '__main__':
    main()
