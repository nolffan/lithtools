# PS2 NOLF models (`.ltb`, file type 2, version 16)

The models of *The Operative: No One Lives Forever* on the PlayStation 2,
in `LITH_PS2.REZ` beside the PC game's names (`chars/models/hero_action.ltb`
for the PC's `CHARS/MODELS/HERO_ACTION.ABC`). `lithtech/ps2ltb.py` reads
and writes them; every model on the US and European discs (1,335 files,
668 and 667) writes back byte for byte (`tests/roundtrip_ps2ltb.py`), and
`tools/lithrez`'s `lithtech::ps2ltb` reads them for LithRez Studio's
preview.

The layout was first worked out in the nolfsrc `ps2` branch
(`build/ps2/spec-models.md`, `ps2model.py`, which converts the PS2 models
that branch installs to PC `.abc`). What follows is that layout, checked
against every file, plus what writing the files needed: how the DMA chains
are packed, which header fields can be computed, and what the hashes need.

Little-endian. `str16` is a u16 length and that many bytes. Quaternions
are x, y, z, w; matrices 16 floats row-major with the translation in the
last column, as on the PC.

## Header

| offset | type | field |
|---|---|---|
| 0x00 | u32 | file type, 2 (PS2 worlds are `.ltb` files of other types) |
| 0x04 | u16 | version, 16 |
| 0x06 | 14 bytes | zero |
| 0x14 | u32 | 0x10 |
| 0x18 | 5 × u32 | offsets of the pieces, nodes, child models, animations, sockets |
| 0x2c | u32 | file size |
| 0x30 | u32 | 0x10 |
| 0x34 | 14 × u32 | counts (below) |
| 0x6c | str16 | command string |
| | f32 | internal radius |
| | u32 | hash multiplier for the file's hashed names (1, 2, 4, 9) |
| | u32 | 0 or 1, unknown |
| | u32, f32[n] | LOD distances (LODs − 1 of them) |

The counts, in order: keyframes (all animations), animations, nodes,
pieces, child models (the model itself included), triangles, vertices,
weights, LODs, sockets, weight sets, strings, string bytes, 0. Most can be
computed from the file (`Model.computed_counts`):

| count | computed as | matches |
|---|---|---|
| triangles | strip triangles, all LODs | 1,331 of 1,335 |
| vertices | rigid: strip vertices told apart by position and normal; skeletal: unique vertices | 1,333 |
| weights | rigid: one a vertex; skeletal: the non-zero weights | 1,055 |
| strings, string bytes | the distinct non-empty command, node, animation and child-model names and keyframe commands (the PC header's rule) | 1,309 |
| the others | their lists' lengths | all |

The weights (always at least the computed number) and the odd strings
count were counted on the PC model the PS2 file was made from, which kept
weights the PS2 dropped and names it hashes. The exporter writes the stored
counts back while a model computes as it did when imported, else the
computed ones.

## Names

Piece, animation and weight-set names are stored only as hashes, sockets
as two; node and child-model names are plain strings. The hash is the
archive's: `h = 0; for c in name.upper() ('/' as '\'): h = h + c + h * mult`
(u32), with the file's multiplier (a socket's second hash uses 1). Node
hashes are always their name's.

`lithtech/ps2ltb_names.txt` turns hashes back into names: the candidates
the nolfsrc branch collects (every name in the PC `.abc` models, the
identifiers in the PS2 executables and attribute files, numbered weapon
animations), kept only where they match a hash in some PS2 model
(`tests/build_ps2ltb_names.py`). With a multiplier as small as 2 names
collide ("Report" and "Select"), so the names the game code asks for come
first. It names 99.7% of animations, all sockets and weight sets but 2,
and 85% of pieces (the rest are pieces of PS2-only models: the safe
cracker, the pen dart, the HUD photos). A hash with no name shows as
`#xxxxxxxx` and exports as that hash; a rigid piece with no dictionary
name takes its node's when that hashes the same.

## Pieces

`u32 count`, then per piece: u32 name hash; 12 f32, three RGBA colours
(0.8 0.8 0.8 1, 0.5 0.5 0.5 0, 0 0 0 1 in every file); u32 skin index;
3 × u32 (0, 0, 4 in every file); then each LOD (the header's count, the
same for every piece):

| size | field |
|---|---|
| u32 | mesh type: 4 rigid, 5 skeletal |
| skeletal: 4 × u32 | qwords in the chain, u16 entries in the weight map, unique vertices, bone-map length |
| rigid: 2 × u32 | qwords in the chain, the node the mesh hangs on |
| DMA chain | below |
| skeletal | weight map (u16s, zero-padded to 16 bytes), unique vertices (`x y z 1 nx ny nz 0`, 32 bytes each), bone map (u32 node per local bone), weights (4 × s16, 4096 = 1.0, then 4 × u8 local bone × 4, 0xff unused; 12 bytes each) |

### The DMA chain

What the EE sends the VU1, unchanged: a run of DMA tags, each one VU
buffer. A tag's qword is `qwc | id << 28` (id 1 CNT, and 6 RET on the last)
with two VIF codes in its upper half: FLUSH (`0x11000000`, NOP on the first
tag) and NOP. Then, for each piece of the buffer, a qword of three NOPs and
an `UNPACK V4-32` (`0x6c << 24 | n << 16 | VU address`) followed by its n
qwords; the buffer goes to VU address 80 on. Last a qword of three NOPs and
`MSCALF 0` to run the microprogram.

A buffer is a header qword (`u32 strips − 1, u32 vertices, 0, 0`) and per
strip a GIF tag and 3 qwords a vertex: `x y z 1`, `nx ny nz 0`, `s t q u32`.
The GIF tag is `NLOOP | EOP << 15 | 0x302E4000 << 32` (PRE, PRIM 0x5c:
textured Gouraud triangle strip; packed; 3 registers) and REGS `0x412`
(ST, RGBAQ, XYZF2), `0x8412` when the strip's first triangle goes the other
way; its upper u32 is a small number of unknown meaning (kept). The u32
after q is 0 in most vertices and a small number in others, unknown
(kept; new vertices get 0); it isn't the unique-vertex index. q is 1, the
two w components 1 and 0, in every file.

Strip triangle k (k ≥ 2) is (k−2, k−1, k), swapped when k is odd, swapped
once more in a 0x8412 strip: PC (clockwise) order. No strip continues into
the next buffer.

How the files are packed, which `ps2ltb.write` reproduces for every one:

- each UNPACK holds whole items (the header, a GIF tag, or a vertex's three
  qwords) and at most 255 qwords: a new UNPACK starts where the next item
  wouldn't fit;
- a buffer holds whole strips, at most 844 qwords (header and GIF tags
  included); in all but 12 files (24 LODs) a buffer ends where the next
  strip wouldn't fit. The 12 put a last lone triangle into a buffer of its
  own; the strips' buffer starts are kept with the model, so those files
  write back too.

Rigid strip vertices are in their node's space, skeletal ones in model
space (bind pose) and each a copy of a unique vertex (the same position and
normal in every file). The weight map lists, per unique vertex, `u16 n` and
the qword offsets (from the chain's first tag) of its copies' position
qwords. The weights of a unique vertex sum to 4096; a few files store a
weight above 4096 with a negative one beside it, and some put one node in
two slots.

## Nodes, child models, animations, sockets

Nodes: per node `str16 name`, 16 × f32 bind matrix (model space), u32 name
hash, u32 child count (depth-first order), u16 index, u16 flags. Then
`u32 count` weight sets: u32 name hash, `u32 n`, f32[n].

Child models: `u32 count` (the model itself included), then count − 1
`str16` file names (`base_general.ltb`, `playerbase.ltb`, ...; the PS2
keeps the human animations in four children).

Animations: `u32 count`; per animation 3 × f32 dims, 3 × f32 translation,
u32 name hash, u32 interpolation ms, `u32 keys`, the keys (`u32 time ms,
str16 command`; no two share a time), then per node `u32 0xffffffff` and 16
bytes a key: `s16 x y z flag, s16 qx qy qz qw`. The position is /16 when the
flag is set, else /4096; the quaternion /16384, and not always of unit
length (up to 0.3% off, once 41%). The flag is set per key: 27,424 coarse
keys would have fitted the fine scale.

Sockets: `u32 count`, 48 bytes each: quaternion, position, u32 0, u32
node, u32 name hash (file multiplier), u32 name hash (multiplier 1), u32 0.
