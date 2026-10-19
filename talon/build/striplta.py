#!/usr/bin/env python3
"""Take the LTA (and .tbw) code out of sources taken from Build 69.

    build/striplta.py <file>...

The 2001 tools read and write only the binary .ed format: LTA, the text
format (ltamgr, CLTANode, CLTAFile, CLTANodeBuilder), came later, as did
the .tbw property groups (CPropListContainer). Each file loses its
#include of ltamgr.h / ltasaveutils.h, every declaration (one line ending
in ';') that names an LTA type or a function whose name ends in LTA, and
every function definition whose name ends in LTA or whose parameters name
an LTA type. Forward declarations of the LTA classes stay (they generate
nothing). What was taken out is printed; the commit's diff is the record.
"""
import re
import sys

LTA_TYPES = r'\b(CLTANode|CLTAFile|CLTANodeBuilder|CPropListContainer|CLTAReader|CLTALoadOnlyAlloc)\b'
LTA_NAME = r'\b\w*LTA\b'


def strip(text):
    out, gone = [], []
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        l = lines[i]
        s = l.strip()
        if re.match(r'#\s*include\s*"(ltamgr|ltasaveutils)\.h"', s, re.I):
            gone.append(s)
            i += 1
            continue
        is_fwd = re.match(r'class\s+\w+\s*;', s)
        mentions = re.search(LTA_TYPES, l) or re.search(LTA_NAME + r'\s*\(', l)
        if mentions and not is_fwd and not s.startswith(('//', '*')):
            # a declaration on one line
            if s.endswith(';') and '{' not in s:
                gone.append(s)
                i += 1
                continue
            # a definition: from here to its matching closing brace
            j, depth, seen = i, 0, False
            while j < len(lines):
                for c in lines[j]:
                    if c == '{':
                        depth += 1
                        seen = True
                    elif c == '}':
                        depth -= 1
                if seen and depth == 0:
                    break
                if not seen and lines[j].rstrip().endswith(';'):
                    break
                j += 1
            gone.append(s + (' ... }' if seen else ''))
            i = j + 1
            while i < len(lines) and not lines[i].strip() and out and not out[-1].strip():
                i += 1
            continue
        out.append(l)
        i += 1
    return '\n'.join(out), gone


def main():
    for p in sys.argv[1:]:
        t = open(p, newline='').read()
        nt, gone = strip(t)
        if gone:
            open(p, 'w', newline='').write(nt)
            print(f'{p}: {len(gone)} taken out')
            for g in gone:
                print('   ', g[:100])


if __name__ == '__main__':
    main()
