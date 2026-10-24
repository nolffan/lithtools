#!/usr/bin/env python3
"""Compares lithtech::sound's PS2 bank decoding (sound_decode_all
--dump-bank) with the nolfsrc ps2 branch's build/ps2/ps2sound.py, sound by
sound: the samples must be equal.

    compare_lps.py <build dir> <dir holding ps2sound.py> <work dir> <file.lps | folder>...

Runs one process per CPU; a folder is searched for .lps files.
"""
import array
import multiprocessing
import subprocess
import sys
from pathlib import Path


def banks(args):
    for a in map(Path, args):
        if a.is_dir():
            yield from sorted(p for p in a.rglob('*') if p.suffix.lower() == '.lps')
        else:
            yield a


def check(job):
    build, ref, work, i, bank = job
    sys.path.insert(0, ref)
    import ps2sound
    out = Path(work) / str(i)
    subprocess.run([str(Path(build) / 'tests/sound_decode_all'), '--dump-bank', str(bank), str(out)],
                   check=True, capture_output=True)
    b = ps2sound.load(str(bank))
    bad = []
    for s in b.sounds:
        ours = array.array('h', (out / f'{s.index}.raw').read_bytes())
        theirs = array.array('h', ps2sound.decode(s.adpcm))
        if ours != theirs:
            bad.append(f'{bank} #{s.index} {s.path}: {len(ours)} vs {len(theirs)} samples')
        (out / f'{s.index}.raw').unlink()
    return len(b.sounds), sum(len(s.adpcm) // 16 * 28 for s in b.sounds), bad


def main():
    build, ref, work = sys.argv[1:4]
    jobs = [(build, ref, work, i, str(b)) for i, b in enumerate(banks(sys.argv[4:]))]
    sounds = samples = 0
    bad = []
    with multiprocessing.Pool() as pool:
        for n, m, b in pool.imap_unordered(check, jobs):
            sounds += n
            samples += m
            bad += b
    for b in bad[:20]:
        print('MISMATCH', b)
    print(f'{len(jobs)} banks, {sounds} sounds (~{samples / 1e6:.0f}M samples): {sounds - len(bad)} equal, {len(bad)} differ')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
