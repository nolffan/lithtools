# The .rez format

Every LithTech `.rez` from Shogo and Blood 2 (1998) to Tron 2.0 and
Contract J.A.C.K. (2003) is **RezMgr version 1**. All 100 we have (NOLF in
every language, GOTY and Mac; AvP2 and Primal Hunt; NOLF2 PC and Mac;
Contract J.A.C.K.; Tron 2.0; and the tools and Jupiter Build 51/69 trees)
use this one layout. No other version number appears in any file or in the
RezMgr sources ("only 1 is possible here right now", `rezmgr.h`).

Sources:
- **talon:** `talon/src/LT2/tools/LithRez`, the 1998 RezMgr and LithRez
  1.00. It is decompiled from the shipped `lithrez.exe` and matches it.
- **Build 69:** `jupiter/src/Engine/libs/rezmgr` and `tools/LithRez`
  (LithRez 1.10, 2004).
- **The files:** every claim below holds for all 100.

All values are little-endian dwords unless noted.

## Header (168 bytes)

| Offset | Size | Field |
|---|---|---|
| 0 | 2 | CR LF |
| 2 | 60 | `RezMgr Version 1 Copyright (C) 1995 MONOLITH INC.`, padded with spaces |
| 62 | 2 | CR LF |
| 64 | 60 | user title: `LithTech Resource File` from LithRez; blank in Shogo/Blood 2 and some later files |
| 124 | 3 | CR LF, 0x1A |
| 127 | 4 | format version: 1 |
| 131 | 4 | root directory position |
| 135 | 4 | root directory size |
| 139 | 4 | root directory time: 0 from LithRez 1.00; uninitialised memory from 1.02/1.10 (the field is never set) |
| 143 | 4 | next write position: the end of the resource data |
| 147 | 4 | time of the last change |
| 151 | 4 | largest key array (entries) |
| 155 | 4 | largest directory name, including its NUL |
| 159 | 4 | largest resource name, including its NUL |
| 163 | 4 | largest comment, including its NUL |
| 167 | 1 | is sorted: 1 in every shipped file |

RezMgr's `Open` checks only the line breaks and the 0x1A. LithRez also wants
the user title, or one of the files every Shogo/Blood 2 archive has
(`cshell.dll`, `cres.dll`, `sres.dll`, `object.lto`, `patch.txt`, ...).

## Directory blocks

A directory is a block of entries: first its subdirectories, then its
resources grouped by type. An empty directory has size 0.

Directory entry:

```
dword 1
dword position, size      of the subdirectory's block
dword time
char  name[]              NUL-terminated
```

Resource entry:

```
dword 0
dword position, size      of the data (position 0 when size is 0)
dword time
dword id
dword type                the extension, reversed, as a dword: "DTX" = 'X','T','D',0
dword number of keys
char  name[]              NUL-terminated, without the extension
char  comment[]           NUL-terminated; empty in every shipped file
dword keys[]              none in any shipped file
```

## Layout as LithRez writes it

1. **Header**, written last.
2. **Data**, in the order the files were added: the directory listed in
   Windows' order (by upper-cased name), each subdirectory taken whole
   where it falls in the listing.
3. **Directory blocks**, each after its subdirectories', with the root
   last. Each block is followed by a **gap of its own size**:
   `CRezDir::WriteAllDirs` adds the block's size to the write position
   after `WriteDirBlock` has already moved it past the block. A clean write
   leaves the gaps zero. 24 shipped NOLF files have leftover disk data in
   them (Windows 9x on FAT doesn't zero a file that grows). The last
   block's gap is never written, so the file ends with the root block.

Inside a block the entries are in RezMgr's hash-table order. Directories go
into 5 bins by name length, types into 9 bins by type value, and each
type's resources into 19 bins by name length. Every new entry goes first in
its bin, and the bins are walked in order.

On create, LithRez 1.00:
- upper-cases names (not with `l`);
- puts an extension of more than four characters into the name, with type 0
  and a warning;
- takes an all-digit name's number as its ID (as `atol`); other files count
  up from 10,000,000, starting again in each directory;
- stamps the clock on every resource. `SetTime(file time)` is followed by
  `Create` and `Save`, which both mark the current time, so a file's own
  date is never kept.

LithRez 1.02 and later write 0 for every ID, and 1.10 adds the extension
filter.

## How lithrez was checked

- **Reading:** all 100 shipped files read and write back byte for byte with
  `lithrez roundtrip`. For the 24 with leftover bytes in their gaps, only
  those bytes differ (`tests/roundtrip_shipped.py`).
- **Creating:** with time fields masked, the same trees give byte-identical
  archives to nine original LithRez builds run under wine
  (`tests/compare_original.py`): LithRez 1.00 (talon's build, NOLF tools
  001, AvP2 tools), 1.02 (NOLF2 toolkit, Contract J.A.C.K. tools) and 1.10
  (Tron tools, Build 69). The trees were a synthetic one (case, digit
  names, long extensions, empty files and folders, all hash bins),
  NOLFTOOLS001.REZ unpacked, and AvP2's AVP2L.REZ unpacked. The tenth
  build, Build 51's LithRez 1.01, is broken: it writes the data but no
  directories and an all-zero header.

## Not RezMgr

Later Jupiter EX games (F.E.A.R., Condemned) use `.Arch00` archives, a
different format. lithrez doesn't read them.
