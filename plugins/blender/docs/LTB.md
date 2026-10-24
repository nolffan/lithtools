# LTB models

The binary models of the Jupiter-era LithTech games: No One Lives Forever 2
(2002), Contract J.A.C.K. (2003) and Tron 2.0 (2003). `lithtech/ltb.py`
reads and writes them.

Every distinct `.ltb` model we have reads and writes back byte for byte:
2,304 files, gathered from NOLF2 (PC and Mac, with the map packs and 1.3
update), Contract J.A.C.K., Tron 2.0, and the NOLF2 toolkit, Contract
J.A.C.K. tools, Tron tools and Jupiter Build 51/69 trees
(`tests/extract_ltb.py`, `tests/roundtrip_ltb.py`).

| Layout | Model file version | Files |
|---|---|---|
| D3D model | 18 | 459 |
| D3D model | 20 | 1 |
| D3D model | 21 | 10 |
| D3D model | 22 | 9 |
| D3D model | 23 | 1,782 |
| D3D model | 24 | 6 |
| D3D model | 25 | 6 |
| chunked | (LTB 3 or 8 or 9) | 31 |

Render style `.ltb` files (LTB file type 5) share the extension but aren't
models; they aren't covered here.

Evidence, in order of weight:

- **Build 69's engine**: `jupiter/src/Engine/runtime/model/src/model_load.cpp`
  (Model::Load and the pieces, nodes, animations, sockets and bindings),
  `modelallocations.cpp`, `model.h` (the channel interpreters, versions),
  `ltb.h`, and the D3D render objects in
  `runtime/render_a/src/sys/d3d/d3dmeshrendobj_{rigid,skel,vertanim}.cpp`
  and `d3d_utils.h` (vertex layouts). NOLF2's and Contract J.A.C.K.'s
  reconstructed `model_load.cpp` (`~/src/nolf2src`, `~/src/cjacksrc`) are
  the same code.
- **Build 69's tools**: `Engine/tools/shared/model/modelallocations.cpp`
  (how the packer counts allocations).
- **The files**: version 18 to 22 aren't in any source we have (the
  engine's minimum is 23); their differences were found by reading every
  file of each version to the end and writing it back unchanged.

All values are little-endian. A string is a uint16 length and that many
bytes (CGenLTStream::ReadString). Vectors are 3 floats, rotations 4 floats
(x, y, z, w), matrices 16 floats row by row with the translation in the
last column.

## D3D model

```
LTB_Header                  20 bytes (ltb.h, as MSVC lays the struct out)
  uint8   file type         1 = LTB_D3D_MODEL_FILE
  uint8   (pad)
  uint16  LTB version       9 (CD3D_LTB_LOAD_VERSION)
  uint8   reserved, 3 pad, uint32 reserved x3   (zero)
uint32  model file version  18 .. 25
uint32  allocation counts x15 (x14 before version 23)
string  command string
float   radius              m_VisRadius
uint32  OBB count, OBBs
uint32  piece count, pieces
node tree
uint32  weight set count, weight sets
uint32  child model count (the model itself is the first), child models
uint32  animation count, animations
uint32  socket count, sockets
anim bindings, one list per child model
```

The file ends there; the engine checks nothing after it.

### Allocation counts (ModelAllocations)

keyframes, parent animations, nodes, pieces, child models, triangles,
vertices, vertex weights, LODs, sockets, weight sets, strings, string
lengths, vertex animation data size, animation data size (the last from
version 23).

The engine sizes one block of memory from these (CalcAllocationSize), and
the animation data has to fit exactly (an assert at the end of loading).
In every file, keyframes, animations, nodes, child models, sockets, vertex
animation data and animation data are what the contents give
(`compute_alloc_counts`, `ALLOC_EXACT`). The rest are hints the engine
doesn't size by: triangles and vertices count the packer's source mesh,
not the D3D one; animation-only models keep the piece counts of pieces the
packer dropped; strings and their lengths are counted over a de-duplicated
list (ModelAllocations::InitFromModel) and are sometimes a few bytes off
either way, which the engine's slack per string covers.

Animation data is, per track, 12 bytes per float position, 16 per float
rotation, 6 per 16-bit position and 8 per 16-bit rotation (the channels'
GetDataSize).

### OBBs

None of the shipped game models has one. Version 18: position, size,
rotation (4 floats) and a node index (44 bytes, found in the files).
Versions 20 to 23: ModelOBB_Depricated, position, size, a 3x3 basis and a
node index (64 bytes). From 24 (MIN_MODELOBB_FILE_VERSION): ModelOBB, the
same and a radius.

### Pieces

```
string  name
uint32  LOD count
float   LOD distances[LOD count]
uint32  min LOD offset, max LOD offset   ("no longer used")
per LOD:
  uint32  texture count
  int32   textures[4]       skin slots (MAX_PIECE_TEXTURES)
  int32   render style      index into the model's render styles
  uint8   render priority   (from version 20)
  uint32  render object type   4 rigid, 5 skeletal, 6 vertex animated, 7 null
  render object
  uint8   used node count, uint8 nodes[count]   (from version 22)
```

The used-node list is the terminal nodes among the mesh's bones (those
none of whose children are also used), ascending (CD3DSkelMesh::
CalcUsedNodes; a rigid mesh's is its node). The engine recomputes it on
load.

### Render objects

Each begins with a uint32 byte count of what follows (the server and a
null mesh skip by it: CDIModelDrawable::Load).

**Rigid (4)**: vertex count, triangle count, max bones per triangle (1),
max bones per vertex (1), 4 stream flags, bone effector (the node), the
vertex streams, the indices (uint16, 3 per triangle).

**Skeletal (5)**: vertex count, triangle count, max bones per triangle, max
bones per vertex, re-indexed bones (bool, 1 byte), 4 stream flags, use
matrix palettes (bool, 1 byte). With matrix palettes: min bone, max bone,
(if re-indexed) a uint32 count and that many bone indices, the streams and
the indices. Without (every shipped model): the streams, the indices, a
uint32 bone set count and the bone sets (BoneSetListItem: uint16 first
vertex, uint16 vertex count, 4 node bytes with 0xFF for none, uint32 end of
its triangles in the index list).

**Vertex animated (6)**: vertex count, un-duplicated vertex count, triangle
count, max bones per triangle, max bones per vertex, 4 stream flags,
animated node, bone effector, the streams, the indices, a uint32 count and
the duplicate map (uint16 source, uint16 copy).

**Null (7)**: nothing (the byte count is 0).

Each of the 4 vertex streams with non-zero flags holds every vertex in
turn (GetVertexFlags_and_Size):

```
if POSITION and NORMAL (0x1, 0x2):
  float position[3]
  float weights[n]          n = bones blended - 1 (D3D's betas)
  uint8 bone indices[4]     matrix palettes only (D3DFVF_LASTBETA_UBYTE4)
  float normal[3]
float uv[2] per UV set      UVSETS_1..4 (0x10..0x80)
float basis[6]              BASISVECTORS (0x100); 3 in Build 69's sphere_basis sample
```

DIFFUSE and PSIZE add nothing. Skeletal meshes without palettes blend
`max bones per triangle` bones (D3D's B1..B3 types); with palettes,
`max bones per vertex`. Every shipped mesh has one stream, flags 0x13
(position, normal, one UV set), except 5 with basis vectors and the 35 null meshes.

Vertices are in model space at the bind pose. A bone set's draw sets its
nodes' transforms in order and stops at the first 0xFF
(SetTransformsToBoneSet), so a set of k bones blends with k - 1 weights
and the last bone gets 1 minus their sum; weight slots past k - 1 hold
leftovers (in 23,042 of 38,490 sets), kept as stored.

One Tron model (FX/MODELS/RECTORSCRIPT_SHARDS) has skeletal meshes with 0
bones per triangle, which the engine refuses (Load_RD asserts); their
vertices are 20 bytes each, kept as bytes.

### Nodes

Depth first, parent before children: string name, uint16 index, uint8
flags, the global bind matrix, uint32 child count, children
(ModelNode::Load).

### Weight sets

String name, uint32 count (= nodes), floats.

### Child models

From version 20: the file name of each (the model itself, first, has none).
Version 18: as in `.abc`, each entry (the model's own first) has a uint32
save index and a (position, rotation) per node, the later ones after their
names.

### Animations

```
vector  dims
string  name
uint32  compression         0 none, 1 relevant keys, 2 16-bit, 3 16-bit rotations
uint32  interpolation ms
uint32  keyframe count, keyframes (uint32 time ms, string)
a track per node, depth first
```

Times needn't be distinct: some weapon and cinematic animations have two or
more keyframes at the same millisecond.

Tracks (AnimNode::Load):

- **None (0)**: uint8 vertex animation flag. With it, per keyframe a uint32
  count and that many float triples (the un-duplicated vertices). Without:
  a position and a rotation per keyframe; version 18 interleaves them
  (position, rotation per key), later versions store the positions, then
  the rotations.
- **Relevant keys (1), 16-bit (2), 16-bit rotations (3)**: a uint32 count and
  the positions, a uint32 count and the rotations. 16-bit positions are 3
  int16s in 1/16 units (NKF_TRANS_SCALE_1_11_4); 16-bit rotations 4 int16s
  over 32767. Kind 3 keeps float positions.
  - Versions 23 and later: no keys means identity, one key holds for every
    keyframe, otherwise one per keyframe (the channel interpreters).
    Shipped files never keep none; a channel keeps one key when its floats
    never change, before they are quantised (102 channels have equal
    16-bit values in every key and still keep them all).
  - Versions 21 and 22: a uint32 (always 0) after each non-zero count.
  - Versions 18 and 20: each key is stored with its time (uint32 ms) before
    its value, and only some keys are kept; between them the value is
    interpolated. The nth keyframe at a repeated time takes the nth key at
    that time.

Transforms are relative to the parent node: composing the first key of a
model's base animation down the tree gives the bind matrices back (within
the 16-bit quantisation).

Some rotations aren't unit quaternions (a few sockets at length 0.707, and
Build 51's IsFlag animates the zero quaternion and (1, 0, -10, 0)); they
are kept as stored.

### Sockets

uint32 node, string name, rotation, position, scale (vector).

### Anim bindings

Per child model (the model itself first): a uint32 count, then per
animation its name, dims and translation (Model::LoadAnimBindings).

## Chunked files

31 files in NOLF2's and Contract J.A.C.K.'s FX folders (and the same in the
Mac build) are a chain of named chunks like `.abc`'s (a string name, the
offset of the next chunk or 0xFFFFFFFF, the body): `LTBHeader`, `Header`,
`Pieces`, ... Their first byte, read by the engine as the LTB file type,
is 9, so Model::Load refuses them. `ltb.ChunkedModel` keeps the chunks as
they are and writes them back.
