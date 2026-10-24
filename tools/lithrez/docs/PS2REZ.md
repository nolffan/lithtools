# The PS2 resource archive (LITH_PS2.REZ)

The Operative: No One Lives Forever on the PS2 (US disc SLUS-20028, European
SLES-50592) keeps all its game data in one file, `LITH_PS2.REZ` (3.9 GB),
in the disc's root. It isn't a RezMgr file: it has 2048-byte sectors, no
directory tree and no names, only 32-bit hashes of them. `lithtools`
reads it (`lithtech::ps2rez`, and `rez::Archive::open` as a tree) straight
out of a disc image or as a file of its own, and can edit it
(experimentally: see the end).

The layout below is from the IOP module that reads the archive,
`LTIOREZ.IRX` on both discs, which still has its symbols and the type names
of its debugging information (`RezTOCHeader`, `RezTOCEntry`, `rezCalcHash`,
`RegisterRezFile`, `rezLoadTableInternal`, `rezOpenInternal`), checked
against both discs' archives. The nolfsrc `ps2` branch's `ps2rez.py`
(a port of StenApp's ps2rezdecoder) got the header, tables and hash first.

## Layout

All values are little-endian 32-bit words.

    sector 0           RezTOCHeader (20 bytes), then zeros
    sector 1           table 0
    ...                table t at sector 1 + t * S
    sector m_Offset    data; every file starts on a sector boundary

`RezTOCHeader`:

| Offset | Field | Both discs | Meaning |
|---|---|---|---|
| 0 | `m_Version` | 3 | `RegisterRezFile` refuses any other value |
| 4 | `m_NumFiles` | 6354 (US), 6356 (EU) | entries in every table; refused above 6500 |
| 8 | `m_HashWeight` | 10 | the hash's multiplier |
| 12 | `m_Offset` | 25301 | the first data sector: 1 + tables * S |
| 16 | `m_NumTOC` | 506 | tables |

A table is `m_NumFiles` 16-byte `RezTOCEntry`s, read as
`ceil(m_NumFiles * 16 / 2048)` whole sectors, so S = 50 sectors on both
discs (102,400 bytes, room for 6400 entries); the rest of a table's last
sector is zero.

`RezTOCEntry`:

| Offset | Field | Meaning |
|---|---|---|
| 0 | `m_nHash` | the name's hash |
| 4 | `m_nOffsetSectors` | the file's first sector, counted from the archive's first sector |
| 8 | `m_nFileSize` | bytes |
| 12 | `m_nFileSizeSectors` | `ceil(m_nFileSize / 2048)` in every entry of both discs |

The hash (`rezCalcHash`) runs over the name upper-cased (`toupper`), with
`/` turned into `\`:

    h = 0
    for each byte c:  h = h + c + h * weight      (32-bit, weight = m_HashWeight)

## How the game reads it

`RegisterRezFile` finds the archive with `sceCdSearchFile("\LITH_PS2.REZ;1")`
(ISO 9660), reads its first sector, checks the version and the entry count,
reads table 0 (`ceil(n * 16 / 2048)` sectors from the archive's second
sector) and then adds the archive's own disc sector to every entry's
sector (`ps2rez_Init`). From then on a file is read with `sceCdRead` at
that absolute sector: raw sectors of the disc, not through the file
system.

`rezOpenInternal` looks a name up by its hash with a linear scan of the
table's `m_NumFiles` entries, taking the first match. Order doesn't matter
(table 0's entries happen to be in sector order).

`rezLoadTableInternal` replaces the table with another, on a request from
the game (the EE side) that gives the table's byte offset and size in the
archive; it reads those sectors and adds the archive's sector to every
entry again. The executable picks the table; its code wasn't traced, but
the tables themselves show what they are for:

- table 0 names every file;
- the other 505 are five per level (101 levels, one per asset list in
  `AssetLists\`). Each names the same files as table 0 but points some of
  them elsewhere. Table 5k+1 points that level's files (those its asset
  list names) at copies laid out together, in one run of sectors.
- The other four of a level's tables also point files at *variants*: on
  the US disc, tables 5k+2 to 5k+5 have the PAL screen layout
  (`Attributes\Layout.txt` and `.but`, commented "added 60 in the Y for the
  pal version"), and 5k+2 and 5k+3 other voice banks (`stream\mNN\vo_*.lps`)
  and level banks. Over all tables, 196,693 of the entries that point away
  from table 0 point at identical copies, 75,750 at variants
  (`lithrez verify --data` counts them per extent).

No two names share an extent. Zero-length files are entries with a size of
0 (188 on the US disc, 189 on the European one); none has a recovered name.

## Names

The archive stores only hashes, so lithtools ships a list of names
(`core/data/ps2-names.txt`, compiled into the library), recovered by
`tests/ps2_names.py` from:

1. every path in the PC NOLF archives;
2. every string in the PS2 files and in the PS2 executable;
3. the folders of the names found, combined with every file name seen;
4. identifiers from all of these, and from each text file found, as
   `worlds\X.ltb`, `assetlists\X.but` and `.txt`, `attributes\X.but` and
   `.txt`, and `lps\X.lps`, repeated while text files add names.

The hash is weak (h * 11 + c): related strings give related hashes, and
with about a million candidates against 6354 hashes some candidates match
files they don't name. Six folder-and-file combinations
(`TEX\METAL\MT02\OBJ0511.dtx` to `OBJ0712.dtx`) matched compiled asset
lists, `TEX\OBJECTS\OBJ10\O..OQQ` matched a model and `GUNS\SND\SHELLS\]''.L`
another file. So a name is kept only if its extension is one the games use
and agrees with the file's content (a `.dtx` must start like a DTX, an
`.ltb` like a PS2 model or world, `.lps` "LithTech PSX Sound", `.wav`
"RIFF", `.pcx` a PCX; `.but`, `.txt` and `.spr` must be none of those), and
it has no stray characters. The list has 6042 names: 6040 of the US disc's
6354 files, 6042 of the European disc's 6356. The rest are shown as
`_UNNAMED\<HASH>.<ext>`, the extension guessed from the content.
`lithrez --names <file>` adds names (one per line, or the recovery script's
`HASH<tab>name` lines).

The recovered names spell folders differently (`Chars\`, `CHARS\`,
`chars\`); the hash ignores case, so the tree upper-cases folder names (as
the PC games' archives have them) and keeps each file name's case.

## Compiled attribute files (.but)

Next to each text attribute file and asset list (`Attributes\Weapons.txt`,
`AssetLists\m01s01.txt`) the archive has a compiled one (`.but`), the form
the PS2 loads. All little-endian:

    u32 tag count
    per tag: u32 name hash, u32 offset of its keys, u32 key count
    keys:    u16 record size (8 + value, a multiple of 4), u16 type, u32 name hash, value

| Type | Value | Text form |
|---|---|---|
| 1 | string, NUL-terminated, padded | `"..."` |
| 2 | s32 | `1` |
| 3 | float | `0.75` |
| 4 | 3 floats | `<x, y, z>` |
| 5 | 2 x s32 | `(x, y)` |
| 6 | u32, 0 or 1 | `TRUE` / `FALSE` |
| 7 | 2 floats | `[a, b]` |
| 8 | 4 x s32 | `(left, top, right, bottom)` |

The types were matched against the text files' values (`HelpSize = (13,23)`
is type 5, `AttackVectorRange = [100.0, 500.0]` type 7). Tag and key names
are stored only as hashes: `h = h * 31 + c` over the name as written
(case-sensitive). Every `[Tag]` and `Key =` in the PS2's text files and the
PC games' attribute files, plus one (3983 names, `core/data/ps2-bute-names.txt`,
written by `tests/ps2_names.py`), names every tag and key of the 125 `.but`
files on each disc. The one is `FlatItl`: the compiled `Layout` of the PS2
variant tables (an unnamed `.but`) was built from a source that spelled the
key `FlagItl` that way, so the game, which asks for `FlagItl`, never finds
the Italian flag's texture there. `lithtech::butes` turns them back into text
(`lithrez cat --text`, and LithRez Studio's preview); it doesn't write them.

## Editing (experimental)

Nothing written here has been run on a PS2 or an emulator. What the writer
does follows from the module's code above:

- **Unchanged**: written back byte-identical (the file is copied; the
  tables written over it are the same bytes). `lithrez roundtrip` also
  rebuilds an archive from its tables alone (header, tables, every extent
  at its sector, zeros elsewhere), and both discs' archives come out
  byte-identical, so there is nothing in them outside the tables' extents.
- **Replacing a file** replaces table 0's version and every table's
  identical copy of it; variants stay. If the new data fits every copy's
  sectors it is written over them (the rest zeroed) and the entries keep
  their sectors; otherwise it is written once at the end and those entries
  point there.
- **Adding a file** appends an entry to every table. The tables' last
  sector has room for 46 more entries (6400 per table) without moving the
  tables or the data; beyond that, lithrez refuses (the module itself
  accepts up to 6500, but the tables would grow into the data).
- **Removing a file** takes its entry out of every table.
- **Renaming or moving** gives each table's entry (copies and variants)
  the new name's hash; the data stays where it is.
- **Names** that the list doesn't have are kept in the archive itself, in
  `LITHTOOLS\NAMES.TXT`, which lithrez reads when it opens the archive (the
  game never asks for it).
- **In a disc image**, data written at the end goes after the volume's
  last sector (the archive isn't the last file on either disc), and the
  primary volume descriptor's volume space size grows to cover it. The
  module reads raw sectors from the archive's start, so an entry may point
  past the archive's own extent; the archive's directory record isn't
  changed. The discs also have a UDF bridge, which isn't updated: a UDF
  reader still sees the original volume (the PS2 uses ISO 9660). The image
  grows by the size of whatever didn't fit in place; a DVD-5 (2,295,104 sectors)
  has room for about 330,000 more on the US disc and 319,000 on the
  European one (620 to 640 MiB).

What stays unverified: that the game accepts any of this (the EE side's
handling of tables and requests wasn't traced), that a disc image with
data past its original volume boots and reads on an emulator or a burned
disc, and how the executable chooses a level's table.

## Checks

- `tests/ps2_verify.py <lithrez> <work dir> <reference> <images...>`: on
  both discs, every file extracts byte-identical to a reference extraction
  (6166 + 188 empty, 6167 + 189); the archive rebuilt from its tables is
  byte-identical; replacing (in place and appended), adding and removing
  files leaves `lithrez verify --data` clean, with the changed files reading
  back and the rest unchanged; and a replacement in a copy of the disc
  image grows the volume and reads back.
  Every `.but` file decodes as text with all its names.
- `ps2rez_test` (ctest): a small archive made in the test, with a copy and
  a variant in its other tables, read and edited through `rez::Archive`.
