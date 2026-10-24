# DTX textures

LithTech's texture file, version -5. Every `.dtx` we have, from NOLF (2000)
to Tron 2.0 (2003), is version -5 and uses this one layout: 21,955 distinct
files across NOLF (all languages and GOTY), AvP2 Primal Hunt, NOLF2,
Contract J.A.C.K., Tron 2.0, the Tron 2.0 tools and the Jupiter Build 51/69
game trees. All of them read and write back byte for byte with
`lithtech/dtx.py` (one 0-byte Tron file is not a texture; see
[Odd files](#odd-files)).

Evidence, in order of weight:

- **talon**: `talon/src/LT2/tools/shared/engine/dtxmgr.h`, `dtxmgr.cpp`
  and `pixelformat.h`/`.cpp`. This is the engine code the 2001 tools
  (DEdit, Processor) were built with. `dtx_Create` reads the format and
  `dtx_Save` writes it.
- **Build 69**: `jupiter/src/Engine/runtime/shared/src/dtxmgr.*` and
  `pixelformat.cpp`, the same format two years on. Also DEdit's
  `jupiter/src/Engine/tools/DEdit/TextureDlg.cpp` and
  `Misc/TextureHelper.cpp`, which write the palette and cube-map sections.
- **The files themselves**: every claim below was checked against all
  21,955 files (`tests/roundtrip_dtx.py`).

All values are little-endian.

## Layout

```
DtxHeader                   164 bytes
pixels of mip 0..n-1        CalcImageSize(bpp, w>>i, h>>i) bytes each
section 0..m-1              32-byte SectionHeader + m_DataLen bytes
```

Nothing follows the last section in any file.

The comment in `dtxmgr.h` describes something different: alpha masks after
the mips, and a "1 if more sections, 0 if no more" word before each
section. That comment is out of date. `dtx_Create` reads exactly
`m_nSections` sections, back to back, with no marker, and `dtx_Save` writes
them that way. The files agree.

## DtxHeader (164 bytes)

| Offset | Type | Field | Notes |
|---|---|---|---|
| 0 | uint32 | `m_ResType` | `LT_RESTYPE_DTX` = 0; `dtx_Create` rejects anything else |
| 4 | int32 | `m_Version` | -5 (`CURRENT_DTX_VERSION`); `dtx_Create` rejects anything else |
| 8 | uint16 | `m_BaseWidth` | |
| 10 | uint16 | `m_BaseHeight` | |
| 12 | uint16 | `m_nMipmaps` | 1..15 accepted by talon's `dtx_Create`; Build 69 caps it at `MAX_DTX_MIPMAPS` = 8. Shipped files use 1 to 8 (mostly 4) |
| 14 | uint16 | `m_nSections` | valid only with `DTX_SECTIONSFIXED` |
| 16 | int32 | `m_IFlags` | see [Flags](#m_iflags) |
| 20 | int32 | `m_UserFlags` | defined by the game; see below |
| 24 | uint8[12] | `m_Extra` | see [m_Extra](#m_extra) |
| 36 | char[128] | `m_CommandString` | NUL-terminated, then garbage (see below) |

`m_UserFlags`: Processor copies it into each world texture's info as a
`uint16` (`GetWorldTextureInfo`, talon `PreProcessor/gettextureinfo.cpp`).
Shipped values are small numbers (0x14, 0x1e, 0x0a, 0x07…), which look like
the games' surface type ids. That reading is not confirmed. The library
keeps the field as a plain int.

**Command string.** Shipped command strings hold render hints such as
`Detailtex Tex\DetailTex\Dtl0001.dtx`, `EnvMap Tex\EnvMap\EnvMap011.dtx`,
`alpharef 96` and `mipmapbias -1`. 388 different strings occur.

In about half the files, the bytes after the NUL are not zero. They are
leftover memory from the tool that saved the file: `dtx_Save` memcpys the
whole in-memory header. The library keeps all 128 bytes as
`command_raw`, and `command` gives you the text.

### m_IFlags

From `dtxmgr.h`. `dtx_Save` masks the field with `DTX_FLAGSAVEMASK`.

| Bit | Name | |
|---|---|---|
| 0 | `DTX_FULLBRITE` | |
| 1 | `DTX_PREFER16BIT` | |
| 2 | `DTX_MIPSALLOCED` | runtime only; not saved |
| 3 | `DTX_SECTIONSFIXED` | set in every file. Without it, `dtx_Create` reads no sections and treats the count as 0 |
| 6 | `DTX_NOSYSCACHE` | |
| 7 | `DTX_PREFER4444` | |
| 8 | `DTX_PREFER5551` | |
| 9 | `DTX_32BITSYSCOPY` | |
| 10 | `DTX_CUBEMAP` | faces 2-6 are in a `CUBEMAPDATA` section |
| 11 | `DTX_BUMPMAP` | |
| 12 | `DTX_LUMBUMPMAP` | |

Shipped combinations: 0x8, 0x9, 0x108, 0x88, 0x10a, 0x109, 0xa, 0x89, 0x408,
0x8a, 0x808, 0x1008, 0xb and 0x288.

### m_Extra

The meanings come from the `DtxHeader` comment and its accessors in
`dtxmgr.h`.

| Byte | Meaning | Shipped values |
|---|---|---|
| 0 | texture group (Build 69 `dtx_Create` adds `g_CV_TextureGroupOffset[group]` to the mip offset) | 0-10, one 16, one 100 |
| 1 | mipmaps to use (set by DEdit's texture properties) | 0 in 11,072 files (NOLF-era tools left it unset); otherwise usually `m_nMipmaps` |
| 2 | BPPIdent; 0 means `BPP_32` (`GetBPPIdent`) | 0, 3-7 |
| 3 | non-S3TC mipmap offset | 0, 1, rarely 2-7 |
| 4 | UI mipmap offset; scale = `1 << n` | 0-2 |
| 5 | texture priority | always 0 |
| 6-9 | detail texture scale, a float stored as value - 1.0 (so zeroed bytes mean 1.0) | 1.0, 6.0, 4.0, 5.0, … |
| 10-11 | detail texture angle, int16 | 0, 90, rarely 1 or 45 |

## Pixel formats

BPPIdent values come from `pixelformat.h`. Level sizes come from
`CalcImageSize` in `pixelformat.cpp`, which talon and Build 69 share.

| Id | Name | Bytes per level | Shipped |
|---|---|---|---|
| 0 | `BPP_8P` | w·h | never: a stored 0 reads as `BPP_32` |
| 1 | `BPP_8` | w·h | none |
| 2 | `BPP_16` | w·h·2 | none |
| 3 | `BPP_32` | w·h·4 | yes |
| 4 | `BPP_S3TC_DXT1` | w·h / 2 | yes |
| 5 | `BPP_S3TC_DXT3` | w·h | yes |
| 6 | `BPP_S3TC_DXT5` | w·h | yes |
| 7 | `BPP_32P` | w·h | 6 files (NOLF GOTY, NOLF2) |

- **Mip sizes.** Each mip halves both dimensions with a shift (`dtx_Alloc`).
  S3TC sizes are **not** rounded up to whole 4×4 blocks: by `CalcImageSize`,
  a 2×2 DXT1 level would be 2 bytes. No correctly formed shipped file has a
  compressed level smaller than 4 pixels on a side, so this never comes up.
  The library uses `CalcImageSize` exactly. Its decoder pads a short level
  with zeros. Its encoder truncates to the same size.
- **Row order.** Rows are stored top row first. Decoded skins come out
  upright.
- **BPP_32.** Each pixel is a PValue: a `uint32` 0xAARRGGBB
  (`ltpvalue.h` masks), so the bytes are B, G, R, A.
- **BPP_32P.** One byte per pixel, an index into the `PALLETE32` section
  (spelled that way) of 256 four-byte entries. Talon `CC_32PtoBF` builds
  the PValue as `b<<24 | g<<16 | r<<8 | a` from an
  `RPaletteColor {a, r, g, b}` whose field names are off by one. In memory
  the entries are B, G, R, A, the same as PValue. DEdit's `Write8BitDTX`
  writes the section from a 256 × 4-byte palette.
- **S3TC** blocks follow the standard DXT layout:
  - DXT1: 8 bytes per block, two RGB565 colors and 32 bits of 2-bit indices.
  - DXT3: 8 bytes of 4-bit alpha, then a DXT1 color block.
  - DXT5: two alpha endpoints, 48 bits of 3-bit indices, then a color block.

  The decoder follows the engine's software path (`ConvertDXTGeneric`)
  with one exception:
  - **565 expansion:** `j*255/max`, truncated (`FormatMgr::InitScaleTables`).
  - **Interpolation:** truncating divisions.
  - **DXT3 alpha:** `n*17` (`g_FullAlphaValues`).
  - **DXT1:** 4 colors when c0 > c1. Otherwise 3 colors, and the fourth is
    transparent black.
  - **The exception:** DXT3 and DXT5 always use 4 colors, as the Direct3D
    hardware that drew the games did. The engine's software path tests
    c0 > c1 for these too.
- **BPP_16.** No file says what layout is stored: `dtx_SetupDTXFormat2`
  gives `BPP_16` no masks. The decoder reads RGB565, the format manager's
  `m_RGB565Format`, unless the flags prefer 4444 or 5551. This is unverified.

## Sections

```
char   m_Type[15]     NUL-terminated
char   m_Name[10]     NUL-terminated
(3 bytes of struct padding)
uint32 m_DataLen
uint8  data[m_DataLen]
```

`SectionHeader` in `dtxmgr.h` is 25 bytes of chars and a `uint32`. MSVC
aligns it to 32 bytes, and `dtx_Save` writes `sizeof(m_Header)`. The
padding bytes, and the bytes after each NUL, are leftover memory in many
files. The library keeps them all.

`dtx_Create` links sections in reverse order: it pushes each one onto the
head of the list. `dtx_Save` writes them in list order, so a load and save
in the tools reverses them. The library keeps the file's order.
`dtx_FindSection` matches the type exactly, with `strcmp`.

Section types:

- **`PALLETE32`** (6 files): the 256-color palette for `BPP_32P`, 1024
  bytes.
- **`CUBEMAPDATA`** (76 files, all with `DTX_CUBEMAP`): faces 2-6 of a cube
  map. The base mips in the file are the first face. This section holds the
  other five, each with all its mips at the base face's sizes, concatenated:
  `m_DataLen` = 5 × (sum of mip sizes). The face order comes from the
  `DTX_CUBEMAP` comment, "-x,+y,-y,+z,-z", so the base is +X. DEdit builds
  the section in `TextureHelper.cpp` (`dataSize * 5`).
- **`LIGHTDEFS`**: texture lights. Processor reads them (talon
  `gettextureinfo.cpp`, `ParseLightDefs`) as one command per light:
  `<center|dirlight|omni> <u> <v> [Z:<z>] <radius> [<r g b>] [<r g b>]`.
  No shipped file has this section; it is a source-art feature.

## Mipmaps

DEdit regenerates mips with `dtx_BuildMipmaps` (talon `dtxmgr.cpp`,
called from Build 69 `TextureDlg.cpp` `CreateNewMipTexture`). Each level is
the 2×2 box average of the level above it, per byte, truncated
(`sum >> 2`). `build_mipmaps()` does the same. It reproduces the stored
mips of 5,180 of the 5,562 multi-mip 32-bit files exactly (322 of 330 in
AvP2 Primal Hunt, 1,566 of 1,674 in NOLF). The rest were presumably mipped
some other way.

DEdit's `MaxMipsForSize` allows mips down to 8 pixels. `from_rgba()` uses
that as its default count, capped at 8.

The S3TC compressor the original tools used is not available, so encoding
cannot reproduce a shipped compressed file. Only parsing and writing are
byte-exact. `encode_pixels()` is a simple range-fit compressor.

## Versions

`read()` dispatches on `m_Version` through `_READERS`, and only -5 is
there:

- NOLF, AvP2 and PH, NOLF2, Contract J.A.C.K. and Tron 2.0 all ship -5.
  Talon and Build 69 both define `CURRENT_DTX_VERSION` as -5 and reject
  everything else.
- Earlier engines (Shogo, Blood 2: -2/-3/-4?) and later ones (LithTech
  Jupiter EX and on) are not in our data, and there is no source for them.
  A reader for them goes in `_READERS`/`_WRITERS` beside `_read_v5`.

## Odd files

- `tron2-usa Game2.rez:TEX/PROTOTYPE/M09/DAN/PANGLOWEDIT.dtx` is 0 bytes.
- `tron2-usa Game2.rez:TEX/PROTOTYPE/M06/DAN/FLOORANGLEUV.dtx` (64×1024,
  DXT5, 6 mips) and `GLASSARROWRUNNER.dtx` (32×512, DXT5, 5 mips) are
  missing their last level: 64 bytes for a 2×32 mip. Whatever wrote them
  sized levels in whole blocks, which is 0 bytes for a level 2 wide. The
  engine would fail to load them (`dtx_Create` checks the stream's error
  status). `read()` keeps the short level and sets `truncated`, so they
  still round-trip. `read(data, lenient=False)` raises instead.

## Alpha

Many skins use alpha as a mask rather than as transparency. For example,
AvP2 Primal Hunt's `MERC7_HEAD` is DXT3 with every alpha 0. Importers
shouldn't wire alpha to transparency without the game's render style or
command string to say so.
