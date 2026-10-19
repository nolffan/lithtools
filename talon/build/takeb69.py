#!/usr/bin/env python3
"""Take source files, and every header they include, from Build 69's tree.

    build/takeb69.py <project dir> [-I<dir>]... <source>...

Paths are relative to the Build 69 tree, ../jupiter/src (Engine/..., Libs/...),
and land at the same place under src/. <project dir> is the compile's
working directory (the directory of the .dsp to be); each -I is an include
directory as the project spells it, relative to that directory, with `\\`
or `/`. Every #include of each source and, recursively, of every header it
names is resolved as MSVC would ("name": the including file's directory,
then the -I directories; <name>: the -I directories), case-insensitively,
conditionals ignored, the compiler's own headers skipped (they resolve
nowhere in the tree). A file already under src/ is left alone: talon's
copies are edited after they are taken, and a later take must not undo that.

Prints what it copied. Text gets LF line endings, as jupiter/src has.
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TALON = os.path.dirname(HERE)
B69 = os.path.normpath(os.path.join(TALON, '..', 'jupiter', 'src'))
DST = os.path.join(TALON, 'src')
INC = re.compile(rb'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.M)


def ci(base, rel, isdir=False):
    """Resolve rel under base case-insensitively; None if it is not there."""
    cur = base
    for part in rel.replace('\\', '/').split('/'):
        if part in ('', '.'):
            continue
        if part == '..':
            cur = os.path.dirname(cur)
            continue
        if not os.path.isdir(cur):
            return None
        m = [x for x in os.listdir(cur) if x.lower() == part.lower()]
        if not m:
            return None
        cur = os.path.join(cur, m[0])
    return cur if (os.path.isdir(cur) if isdir else os.path.isfile(cur)) else None


def main():
    args = sys.argv[1:]
    proj = os.path.join(B69, args[0])
    incs = [ci(proj, a[2:], isdir=True) or os.path.normpath(os.path.join(proj, a[2:].replace('\\', '/')))
            for a in args[1:] if a.startswith('-I')]
    srcs = [a for a in args[1:] if not a.startswith('-I')]
    todo = []
    for s in srcs:
        p = ci(B69, s)
        if not p:
            sys.exit(f'not in Build 69: {s}')
        todo.append(p)
    seen, copied = set(), []
    while todo:
        p = todo.pop()
        if p in seen:
            continue
        seen.add(p)
        data = open(p, 'rb').read()
        for kind, name in INC.findall(data):
            name = name.decode('latin-1')
            dirs = ([os.path.dirname(p)] if kind == b'"' else []) + incs
            for d in dirs:
                q = ci(d, name)
                if q:
                    todo.append(q)
                    break
        rel = os.path.relpath(p, B69)
        out = os.path.join(DST, rel)
        if os.path.exists(out):
            continue
        os.makedirs(os.path.dirname(out), exist_ok=True)
        shutil.copyfile(p, out)
        copied.append(rel)
    for c in sorted(copied):
        print(c)
    print(f'{len(copied)} file(s) taken, {len(seen) - len(copied)} already here',
          file=sys.stderr)


if __name__ == '__main__':
    main()
