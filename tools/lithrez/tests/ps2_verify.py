#!/usr/bin/env python3
"""Checks lithrez against the shipped PS2 archives.

    ps2_verify.py <lithrez> <work dir> <reference dir> <disc image>...

For each disc image (or LITH_PS2.REZ):
  1. lithrez x extracts every file, and each one is byte-identical to the
     reference extraction (<reference dir>/<region>, matched by name hash,
     as made by the nolfsrc ps2 branch's reader: see docs/PS2REZ.md);
  1b. every compiled attribute file (.but) decodes to the end (lithrez cat
     --text), with every tag and key named;
  2. lithrez roundtrip rebuilds the archive from its tables alone, and the
     result is byte-identical to the archive on the disc;
  3. on that rebuilt copy, edits go through lithrez add/rm: a file replaced
     in place (it fits its copies), one replaced by bigger data (appended),
     a new file (whose name lithrez keeps in LITHTOOLS/NAMES.TXT), and a
     removal; afterwards lithrez verify passes, the changed files read back
     as written and every other file is unchanged. (Renames are covered by
     ctest's ps2rez_test.)
  4. the same replacement on a copy of the disc image: the data goes after
     the volume's last sector, which grows, and the image still opens.
The reference dir holds ps2/<region>/ trees and ps2-<region>-names.txt files
("HASH<tab>name"); a disc whose name says USA is "usa", else "eu".
"""
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys


def run(*cmd, **kw):
    return subprocess.run([str(c) for c in cmd], check=True, capture_output=True, **kw)


def sha(path_or_bytes):
    h = hashlib.sha1()
    if isinstance(path_or_bytes, bytes):
        h.update(path_or_bytes)
    else:
        with open(path_or_bytes, 'rb') as f:
            for b in iter(lambda: f.read(1 << 20), b''):
                h.update(b)
    return h.hexdigest()


def listing(lithrez, rez):
    return json.loads(run(lithrez, 'list', '--json', rez).stdout)


def rez_slice_sha(image):
    """sha1 of LITH_PS2.REZ's bytes inside a disc image (or the whole file)."""
    with open(image, 'rb') as f:
        f.seek(16 * 2048)
        pvd = f.read(2048)
        off, size = 0, os.path.getsize(image)
        if pvd[1:6] == b'CD001':
            ext, dsize = struct.unpack_from('<I', pvd, 158)[0], struct.unpack_from('<I', pvd, 166)[0]
            f.seek(ext * 2048)
            d = f.read(dsize)
            p = 0
            while p < len(d):
                ln = d[p]
                if ln == 0:
                    p = (p // 2048 + 1) * 2048
                    continue
                name = d[p + 33:p + 33 + d[p + 32]].split(b';')[0]
                if name.upper() == b'LITH_PS2.REZ':
                    off, size = struct.unpack_from('<I', d, p + 2)[0] * 2048, struct.unpack_from('<I', d, p + 10)[0]
                    break
                p += ln
        f.seek(off)
        h, left = hashlib.sha1(), size
        while left:
            b = f.read(min(left, 1 << 20))
            h.update(b)
            left -= len(b)
        return h.hexdigest(), size


def check(ok, what, fails):
    print(('ok    ' if ok else 'FAIL  ') + what)
    if not ok:
        fails.append(what)


def main():
    lithrez, work, ref = sys.argv[1:4]
    fails = []
    for image in sys.argv[4:]:
        region = 'usa' if 'USA' in os.path.basename(image) else 'eu'
        print('==', os.path.basename(image))
        wdir = os.path.join(work, region)
        shutil.rmtree(wdir, ignore_errors=True)
        os.makedirs(wdir)
        # 1. extraction against the reference, by hash
        refname = {}
        for line in open(os.path.join(ref, 'ps2-%s-names.txt' % region)):
            h, n = line.rstrip('\n').split('\t')
            refname[int(h, 16)] = n
        refdir = os.path.join(ref, 'ps2', region)
        unnamed = {}
        for fn in os.listdir(os.path.join(refdir, '_unnamed')):
            unnamed[int(fn.split('.')[0], 16)] = os.path.join('_unnamed', fn)
        items = listing(lithrez, image)
        run(lithrez, 'x', image, os.path.join(wdir, 'x'))
        same = differ = empty = 0
        for it in items:
            h = it['id']
            mine = os.path.join(wdir, 'x', *it['path'].split('/'))
            if it['size'] == 0:
                empty += 1
                continue
            rel = refname.get(h, '').replace('\\', os.sep) if h in refname else unnamed.get(h)
            theirs = os.path.join(refdir, rel) if rel else None
            if theirs and os.path.exists(theirs) and sha(mine) == sha(theirs):
                same += 1
            else:
                differ += 1
                if differ <= 5:
                    print('   differs:', it['path'], theirs)
        check(differ == 0, '%d files identical to the reference extraction (%d zero-length), %d differ'
              % (same, empty, differ), fails)
        shutil.rmtree(os.path.join(wdir, 'x'))
        # 1b. compiled attribute files as text
        buts = [it for it in items if it['type'] == 'BUT' and it['size']]
        ok = unnamed_keys = 0
        for it in buts:
            t = run(lithrez, 'cat', '--text', image, it['path']).stdout
            if t[:1] == b'[':
                ok += 1
                unnamed_keys += len(re.findall(rb'^\[?#[0-9A-F]{8}', t, re.M))
        check(ok == len(buts) and unnamed_keys == 0,
              '%d of %d .but files decode as text, %d unnamed tags or keys' % (ok, len(buts), unnamed_keys), fails)
        # 2. rebuilt from the tables
        rebuilt = os.path.join(wdir, 'LITH_PS2.REZ')
        run(lithrez, 'roundtrip', image, rebuilt)
        want, size = rez_slice_sha(image)
        check(sha(rebuilt) == want and os.path.getsize(rebuilt) == size,
              'archive rebuilt from its tables is byte-identical (%d bytes)' % size, fails)
        # 3. edits through lithrez add/rm on the rebuilt copy
        before = {it['id']: it for it in listing(lithrez, rebuilt)}
        named = [it for it in before.values() if not it['path'].startswith('_UNNAMED') and it['size'] > 3000]
        small, big = named[10], named[20]
        files = os.path.join(wdir, 'files')
        os.makedirs(files)
        d_small = b'replaced in place\n' * 10           # fits its copies
        d_big = os.urandom(big['size'] + 50000)          # doesn't: appended
        d_new = b'a new file\n' * 1000
        for name, data in ((small['path'], d_small), (big['path'], d_big)):
            p = os.path.join(files, os.path.basename(name))
            open(p, 'wb').write(data)
            run(lithrez, 'add', rebuilt, os.path.dirname(name), p)
        open(os.path.join(files, 'NEWFILE.TXT'), 'wb').write(d_new)
        run(lithrez, 'add', rebuilt, 'LITHTOOLS', os.path.join(files, 'NEWFILE.TXT'))
        gone = named[30]
        run(lithrez, 'rm', rebuilt, gone['path'])
        r = subprocess.run([lithrez, 'verify', '--data', rebuilt], capture_output=True, text=True)
        check(r.returncode == 0, 'verify after edits: ' + r.stdout.strip().splitlines()[-1], fails)
        after = {it['path']: it for it in listing(lithrez, rebuilt)}
        cat = lambda p: run(lithrez, 'cat', rebuilt, p).stdout
        check(cat(small['path']) == d_small, 'replaced in place reads back', fails)
        check(cat(big['path']) == d_big, 'replaced with bigger data (appended) reads back', fails)
        check(cat('LITHTOOLS/NEWFILE.TXT') == d_new, 'added file reads back', fails)
        check(gone['path'] not in after, 'removed file is gone', fails)
        changed = {small['id'], big['id'], gone['id']}
        unchanged_ok = all(after[it['path']]['size'] == it['size'] for it in before.values()
                           if it['id'] not in changed)
        sample = [it for it in before.values() if it['id'] not in changed and it['size']][::300]
        orig = {it['id']: it for it in listing(lithrez, image)}
        unchanged_ok = unchanged_ok and all(cat(it['path']) == run(lithrez, 'cat', image, orig[it['id']]['path']).stdout
                                            for it in sample)
        check(unchanged_ok, 'every other file keeps its size; %d sampled read the same' % len(sample), fails)
        os.remove(rebuilt)
        # 4. the disc image: a bigger replacement goes after the volume
        if image.lower().endswith('.iso'):
            copy = os.path.join(wdir, 'disc.iso')
            shutil.copyfile(image, copy)
            vol0 = os.path.getsize(copy) // 2048
            p = os.path.join(files, os.path.basename(big['path']))
            run(lithrez, 'add', copy, os.path.dirname(big['path']), p)
            with open(copy, 'rb') as f:
                f.seek(16 * 2048 + 80)
                vol = struct.unpack('<I', f.read(4))[0]
            r = subprocess.run([lithrez, 'verify', copy], capture_output=True, text=True)
            check(r.returncode == 0 and vol > vol0 and vol * 2048 == os.path.getsize(copy) and
                  run(lithrez, 'cat', copy, big['path']).stdout == d_big,
                  'disc image: data appended after the volume (%d -> %d sectors), reads back' % (vol0, vol), fails)
            os.remove(copy)
        shutil.rmtree(files)
    print('%d failure(s)' % len(fails))
    sys.exit(1 if fails else 0)


if __name__ == '__main__':
    main()
