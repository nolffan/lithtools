#!/usr/bin/env python3
"""Lay out the built Jupiter tools as a distribution.

    package.py <src> <build69-tools> <out>

<src> is jupiter/src (its Built folders hold what vcbuild.py made),
<build69-tools> the Tools folder of the Build 69 package
(extracted/jupiter-b69/x/Tools/Tools), <out> the distribution folder,
created afresh.

The layout is Build 69's own Tools folder: every program and plug-in we
build goes where Monolith's 2006 build of it was (dedit.exe at the top,
packers/WorldPacker.dll, Plugins/3dsmax7/plugins/*.dle,
Plugins/alias/maya7.0/bin/plug-ins/*.mll, ...). Build 69's data files (MEL
scripts, sample render styles, asset-list scripts, ClassHlp.but, the
DtxShellEx install scripts) and its Microsoft runtime DLLs (MFC 7.0/7.1,
the C/C++ runtimes) are copied as they are. A Build 69 binary we do not
build is not copied; MANIFEST.txt lists those, and where every file came
from. README.txt is dist-README.txt from beside this script.
"""
import os
import shutil
import struct
import sys

RUNTIME = ('mfc70.dll', 'mfc71.dll', 'msvci70.dll', 'msvcp70.dll',
           'msvcp71.dll', 'msvcr70.dll', 'msvcr71.dll')


def is_pe(p):
    with open(p, 'rb') as f:
        h = f.read(0x40)
        if h[:2] != b'MZ' or len(h) < 0x40:
            return False
        f.seek(struct.unpack_from('<I', h, 0x3c)[0])
        return f.read(4) == b'PE\0\0'


def main():
    src, b69, out = sys.argv[1:4]
    ours = {}
    for dp, dn, fn in os.walk(src):
        for f in fn:
            p = os.path.join(dp, f)
            parts = [x.lower() for x in p.split(os.sep)]
            # vcbuild.py's outputs: the projects' Built folders, and the
            # "Release Maya70" folders the Maya 7.0 configurations write
            # inside the source tree
            out_dir = 'built' in parts or any(x.startswith('release') for x in parts)
            if not out_dir or os.path.splitext(f)[1].lower() not in (
                    '.exe', '.dll', '.dle', '.dli', '.bmi', '.mll', '.8bi'):
                continue
            if 'debug' in parts:
                continue
            prev = ours.get(f.lower())
            if prev is None or os.path.getmtime(p) > os.path.getmtime(prev):
                ours[f.lower()] = p
    if os.path.exists(out):
        shutil.rmtree(out)
    manifest, missing, used = [], [], set()
    for dp, dn, fn in os.walk(b69):
        dn.sort()
        for f in sorted(fn):
            p = os.path.join(dp, f)
            rel = os.path.relpath(p, b69)
            dst = os.path.join(out, rel)
            if not is_pe(p):
                origin = 'Build 69 (data)'
                srcf = p
            elif f.lower() in RUNTIME or 'runtime' in rel.lower().split(os.sep)[0]:
                origin = 'Build 69 (Microsoft runtime)'
                srcf = p
            elif f.lower() in ours:
                srcf = ours[f.lower()]
                origin = 'built: ' + os.path.relpath(srcf, src)
                used.add(f.lower())
            else:
                missing.append(rel)
                continue
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(srcf, dst)
            manifest.append(f'{rel}\t{origin}')
    extra = sorted(set(ours) - used)
    with open(os.path.join(out, 'MANIFEST.txt'), 'w') as m:
        m.write('# path\twhere it came from\n')
        m.write('\n'.join(manifest) + '\n')
        m.write('\n# Build 69 binaries not built here (not included):\n')
        m.write('\n'.join('#   ' + x for x in missing) + '\n')
        if extra:
            m.write('\n# built here but not in Build 69\'s Tools folder (not included):\n')
            m.write('\n'.join('#   ' + x for x in extra) + '\n')
    shutil.copyfile(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                 'dist-README.txt'), os.path.join(out, 'README.txt'))
    built = sum(1 for l in manifest if '\tbuilt: ' in l)
    print(f'{out}: {len(manifest)} files, {built} of them built here; '
          f'{len(missing)} Build 69 binaries not built: {", ".join(missing)}')
    if extra:
        print('built but unplaced:', ', '.join(extra))


if __name__ == '__main__':
    main()
