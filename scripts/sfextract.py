#!/usr/bin/env python3
"""Unpack an Indigo Rose Setup Factory 6 installer.

    sfextract.py <setup.exe> <outdir> <blast>

The overlay after the PE image starts with 8 magic bytes (E0..E7) and a
count of "header" files (the setup engine irsetup.exe, its script
irsetup.dat, language files, and files the author put in the header), each
a 260-byte name, the compressed size, the CRC-32 of the data, and the data
as a PKWARE DCL implode stream. The payload files follow with no headers of
their own, one implode stream each, in the script's order. irsetup.dat
records each payload file's source path, then later its compressed size and
CRC-32; a stream is named by finding its (size, CRC) in the script and
taking the last source path before it. Files the author chose not to
compress are stored as they are; one is recognised by finding the size
recorded in an unclaimed record together with the CRC-32 of that many bytes.

<blast> is scripts/blast.c, compiled. Every file is CRC-checked.
"""
import os
import re
import struct
import subprocess
import sys
import zlib


def overlay_start(d):
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    ns = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    end = 0
    for i in range(ns):
        o = pe + 24 + opt + 40 * i
        raw, ptr = struct.unpack_from('<II', d, o + 16)
        end = max(end, ptr + raw)
    return end


def blast(exe, path, off, out):
    r = subprocess.run([exe, path, str(off), out], capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError(f'blast at {off:#x}: {r.stderr.strip()}')
    used, _ = map(int, r.stdout.split())
    return used


def main():
    path, outdir, exe = sys.argv[1:4]
    d = open(path, 'rb').read()
    o = overlay_start(d)
    if d[o:o + 8] != bytes(range(0xe0, 0xe8)):
        sys.exit('not a Setup Factory 6 overlay')
    o += 8
    n = struct.unpack_from('<I', d, o)[0]
    o += 4
    hdr = os.path.join(outdir, '_header')
    os.makedirs(hdr, exist_ok=True)
    for _ in range(n):
        name = d[o:o + 260].split(b'\0')[0].decode('latin-1')
        size, crc = struct.unpack_from('<II', d, o + 260)
        o += 268
        out = os.path.join(hdr, name)
        if size:
            blast(exe, path, o, out)
        else:
            open(out, 'wb').close()
        ok = zlib.crc32(open(out, 'rb').read()) == crc
        print(f'header  {"ok " if ok else "BAD"} {name}')
        o += size

    script = open(os.path.join(hdr, 'irsetup.dat'), 'rb').read()
    # A payload record: its source path, then the length-prefixed base name,
    # source directory and extension, a byte, and the file's size.
    recs = []
    for m in re.finditer(rb'([A-Za-z]:\\[\x20-\x7e]+?)(?=[\x00-\x1f])', script):
        src = m.group(1)
        base = src.rsplit(b'\\', 1)[1]
        e = m.end()
        if script[e] != len(base) or script[e + 1:e + 1 + len(base)] != base:
            continue
        e += 1 + len(base)
        e += 1 + script[e]              # source directory
        e += 1 + script[e]              # extension
        size = struct.unpack_from('<I', script, e + 1)[0]
        recs.append((m.start(), src.decode('latin-1'), size))
    done = set()

    def name_at(at):
        before = [r for r in recs if r[0] < at]
        return before[-1] if before else None

    bad = 0
    tmp = os.path.join(outdir, '_stream.tmp')
    while o < len(d):
        rec = None
        try:
            used = blast(exe, path, o, tmp)
            data = open(tmp, 'rb').read()
            at = script.find(struct.pack('<II', used, zlib.crc32(data)))
            if at >= 0:
                rec = name_at(at)
        except RuntimeError:
            pass
        if rec is None:
            # stored: the first unclaimed record whose size and CRC fit
            for r in recs:
                if r[0] in done or not r[2]:
                    continue
                chunk = d[o:o + r[2]]
                if struct.pack('<II', r[2], zlib.crc32(chunk)) in script:
                    rec, used, data = r, r[2], chunk
                    open(tmp, 'wb').write(data)
                    break
            else:
                sys.exit(f'stream at {o:#x}: neither an implode stream nor a stored file')
        if rec is None:
            name = f'_unnamed/{o:08x}.bin'
            bad += 1
        else:
            done.add(rec[0])
            name = rec[1].split('\\', 1)[1].replace('\\', '/')
        dst = os.path.join(outdir, name)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        os.replace(tmp, dst)
        print(f'payload {"ok " if rec else "???"} {len(data):10} {name}')
        o += used
    print(f'done; {bad} unnamed')


if __name__ == '__main__':
    main()
