#!/usr/bin/env python3
"""Copy the LithTech Jupiter tools source out of Build 69 into jupiter/src.

    import_b69.py <unshield-root> <dest>

<unshield-root> is the Build 69 package unpacked by file group
(extracted/jupiter-b69/x: Engine/Engine/..., Libs/Libs/...). <dest> gets the
tree as the setup lays it out on disk -- Engine/ and Libs/ side by side -- so
the projects' relative paths (..\\..\\..\\libs\\stdlith from Engine\\tools\\X)
resolve unchanged.

Taken: Engine/tools, Engine/libs/{ltamgr,rezmgr,ltmem,enginemodellib},
Engine/sdk, Engine/runtime, Libs. Left out: the 3ds Max, Maya and Photoshop
SDKs under tools/Plugins/SDKFiles (Autodesk's and Adobe's, fetched at build
time instead), what a build leaves behind (Libs/built, *.obj, *.exp, *.pch,
*.ncb, *.suo, the Release_* output folders, which also hold Monolith's own
2006 .lib builds of the Maya plug-ins), the Microsoft runtime DLLs in
tools/MiscPack, and Engine/libs/{externalesd,Lib_DShow} (RealArcade and
DirectShow code no tool uses). Text files get LF line endings; everything
else is copied byte for byte. Prints what it skipped, by reason.
"""
import collections
import os
import shutil
import sys

TEXT = {'.c', '.cpp', '.cc', '.h', '.hpp', '.inl', '.rc', '.rc2', '.def',
        '.vcproj', '.sln', '.dsp', '.dsw', '.mak', '.mk', '.bat', '.txt',
        '.idl', '.odl', '.r', '.lta', '.vsh', '.psh', '.htm', '.html', '.reg',
        '.cfg', '.ini', '.bas', '.mel', '.py', '.pl', '.cls', '.but', '.inc'}
SKIP_EXT = {'.obj', '.exp', '.pch', '.ncb', '.suo', '.idb', '.pdb', '.ilk',
            '.sbr', '.bsc', '.aps', '.plg', '.opt', '.res'}
TAKE = [('Engine/Engine/tools', 'Engine/tools'),
        ('Engine/Engine/libs/ltamgr', 'Engine/libs/ltamgr'),
        ('Engine/Engine/libs/rezmgr', 'Engine/libs/rezmgr'),
        ('Engine/Engine/libs/ltmem', 'Engine/libs/ltmem'),
        ('Engine/Engine/libs/enginemodellib', 'Engine/libs/enginemodellib'),
        ('Engine/Engine/sdk', 'Engine/sdk'),
        ('Engine/Engine/runtime', 'Engine/runtime'),
        ('Libs/Libs', 'Libs')]


def skip_reason(rel):
    parts = rel.replace('\\', '/').split('/')
    low = [p.lower() for p in parts]
    if 'sdkfiles' in low:
        return 'plug-in SDKs (fetched at build time)'
    if low[:1] == ['built']:
        return 'build output (Libs/built)'
    if any(p.startswith('release_') or p in ('debug', 'release', 'hybrid')
           for p in low[:-1]):
        return 'build output folder'
    if 'miscpack' in low and low[-1].endswith('.dll'):
        return 'Microsoft runtime DLL'
    if os.path.splitext(low[-1])[1] in SKIP_EXT:
        return 'build output file'
    return None


def main():
    src_root, dest = sys.argv[1:3]
    skipped = collections.Counter()
    copied = 0
    for src, dst in TAKE:
        base = os.path.join(src_root, src)
        for dp, dn, fn in os.walk(base):
            dn.sort()
            for f in sorted(fn):
                p = os.path.join(dp, f)
                rel = os.path.relpath(p, base)
                why = skip_reason(rel)
                if why:
                    skipped[why] += 1
                    continue
                out = os.path.join(dest, dst, rel)
                os.makedirs(os.path.dirname(out), exist_ok=True)
                if os.path.splitext(f)[1].lower() in TEXT:
                    data = open(p, 'rb').read()
                    open(out, 'wb').write(data.replace(b'\r\n', b'\n'))
                else:
                    shutil.copyfile(p, out)
                copied += 1
    print(f'copied {copied} files')
    for why, n in skipped.most_common():
        print(f'skipped {n:5} {why}')


if __name__ == '__main__':
    main()
