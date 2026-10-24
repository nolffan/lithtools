# LithTech `.abc` models, versions 9–12

The model format of NOLF (2000) and Aliens versus Predator 2 (2001), from
the LithTech 2.x ("Talon") generation. Implemented by
[`lithtech/abc.py`](../lithtech/abc.py).
[`tests/roundtrip_abc.py`](../tests/roundtrip_abc.py) reads and rewrites every
shipped file:

| Version | Files | Byte-identical | Identical with header counts recomputed |
|---|---|---|---|
| 12 | 751 (692 NOLF GOTY, 59 AvP2 Primal Hunt) | 751 | 751 |
| 11 | 1 (NOLF) | 1 | 1 |
| 9 | 2 (NOLF) | 2 | 2 |

The files come from NOLF GOTY US (`NOLF.REZ`, `NOLF2.REZ`, `NOLFGOTY.REZ`,
`nolfu003.rez`; 695 files) and the Primal Hunt US install (`AVP2P1.REZ`,
`AVP2X.REZ`; 59 files). [`tests/extract_abc.py`](../tests/extract_abc.py)
extracts them into the gitignored `out/assets/abc/`.

## Evidence

- **R (the reader):** nolfsrc's reconstruction of NOLF's model loader,
  `nolfsrc/src/engine/server/commonmodel.cpp`. It is matched against the
  shipped `server.dll` and `lithtech.exe`, so its reads are the shipped reads.
  Functions:
  - `mod_FindChunk`
  - `Model::LoadHeader`
  - `CModelFileCounts::Read`
  - `Model::LoadChildModels` (the whole reader)
  - `ModelPiece::sub_1001c1a5` and `sub_1001bf85`, shown as `…Q44C` in the file
  - `CModelSub20C::sub_1001bd54` (nodes)
  - `Model::sub_1001c560` and `ModelWeightSet::sub_1001c2bc` (weight sets)
  - `CModelSub1D4::sub_1001bf0d` (child models)
  - `AnimInfo::sub_1001bec9`, `CUnknownD9E4::sub_1001bc69` and `CUnknownD974::sub_1001bb37` (animations)
  - `Model::LoadSockets`
  - `Model::sub_1001c36c` (AnimBindings)

  The reconstruction often names members `m_Unknown…`. R gives the sizes and
  order of the reads; the names come from B69.
- **B69:** Build 69's model code, `jupiter/src/Engine/tools/shared/model/model.h`.
  This is the next generation (LTA/LTB), but it keeps the 2000 classes and
  their member names. It supplies:
  - `NewVertexWeight { float m_Vec[4]; uint32 m_iNode; }`
  - `ModelVert::m_iReplacement`
  - `MNODE_REMOVABLE`, `MNODE_ROTATIONONLY`
  - `ChildInfo::m_Relation`, `m_SaveIndex`
  - `ModelAnim::m_ModelEditWeightSet`, `m_InterpolationMS`
  - `ModelPiece::m_LODWeight`
- **D (data):** properties measured over the 754 files with the scripts behind
  this document. Each is stated with its count.
- **RT (round trip):** the writer below reproduces all 754 files byte for
  byte. This is the only evidence for the parts the engine skips: the writer
  side, ModelEdit's `model_save.cpp`, was not decompiled. The chunk names
  appear as literals in Talon `ModelEdit.exe` (AvP2 Update 2) and in NOLF Tools
  001's `LithExport.dll`.

## Primitives

All values are little-endian.

- `string`: a uint16 length, then that many bytes, with no terminator
  (`ILTStream::ReadString`). The library decodes strings as Latin-1.
- `vec3`: three float32s.
- `quat`: four float32s in the order x, y, z, w. This is `LTRotation::m_Quat`;
  the w is index 3 in R's `quat_ConvertFromMatrix`.
- `mat4`: sixteen float32s, row-major, `m[row][col]`, with the translation in
  column 3. All 10,318 nodes have a last row of (0, 0, 0, 1) [D].

## Chunks

A file is a chain of chunks. Each chunk has:

- `string name`
- `uint32 next`: the file offset of the next chunk's name, or `0xFFFFFFFF` for the last chunk
- the body, which runs up to `next`

The engine seeks to 0 and follows `next` up to 0x200 times, comparing names
(`mod_FindChunk` [R]). All 754 files have the same seven chunks, in this order
[D]:

`Header`, `Pieces`, `Nodes`, `ChildModels`, `Animation`, `Sockets`, `AnimBindings`

No file has a gap between chunks [RT].

### Header

| Type | Field | Notes |
|---|---|---|
| uint32 | version | 9, 10, 11 or 12. The engine refuses anything else [R `LoadHeader`]. |
| uint32 ×13 | counts | Read and not checked [R `CModelFileCounts::Read`]. See below. |
| string | command_string | The ModelEdit command string. Non-empty in 195 files [D]. |
| float | internal_radius | m_GlobalRadius in B69. Positive in every file [D]. |
| uint16 | nLODs | The number of LOD distances. |
| byte ×62 | reserved | Skipped [R]. All zero in every file [D]. |
| float ×nLODs | lod_distances | E.g. 300, 600, 1000, 1500 on NOLF characters [D]. |

**The thirteen counts.** `compute_header_counts` derives each of these. For all
754 files, the derived values are equal to the stored ones [D/RT]:

1. Keyframes, summed over the animations.
2. The number of animations.
3. Nodes.
4. Pieces.
5. Child models, including the self entry.
6. Triangles, over every piece and LOD.
7. Vertices, over every piece and LOD.
8. Vertex weights.
9. LODs, that is `1 + nLODs`.
10. Sockets.
11. Weight sets.
12. The number of distinct non-empty strings among the command string, node
    names, child-model names, animation names and keyframe strings.
13. Those strings' lengths, summed.

The names of the counts are ours. Counts 12 and 13 size the engine's string
pool; the other counts presumably size its allocations.

### Pieces

| Type | Field | Notes |
|---|---|---|
| uint32 | weight pool size | Sizes the engine's weight array (`m_Unknown054` [R]). It equals header count 8 in every file [D]. The library recomputes it. |
| uint32 | nPieces | |
| *piece* ×nPieces | | |

A piece:

| Type | Field | Notes |
|---|---|---|
| uint16 | texture_index | The skin slot: values 0–3 in the data [D]. |
| float | specular_power | 5.0 in 2,323 of 2,389 pieces [D]. |
| float | specular_scale | 1.0 in most pieces [D]. |
| float | lod_weight | **Version ≥ 10 only** [R `m_nFileVersion >= 10`]. This is m_LODWeight [B69]; values 1–5 [D]. |
| uint16 | nReserved | |
| uint32 ×nReserved | reserved | Skipped by the engine [R]. nReserved is 0 in every piece [D]. |
| string | name | |
| *LOD* ×(1 + nLODs) | | The full mesh first, then one per LOD distance [R: the base read, then one per entry of the header's LOD array]. |

A LOD:

| Type | Field | Notes |
|---|---|---|
| uint32 | nTriangles | |
| ×nTriangles × 3 corners | float u, float v, uint16 vertex | Each corner carries its own UV. |
| uint32 | nVertices | |
| *vertex* ×nVertices | | |

R reads the triangle block as "dims" of `(float, float, uint16) × 3`; the sizes
match. The data shows that this block is the triangles:

- All 809,324 triangles index below nVertices [D].
- 99.8% of the UV coordinates fall in 0–1 [D]. A few outliers reach
  −4.3e8, which is junk in some exported files that is kept as stored.

A vertex:

| Type | Field | Notes |
|---|---|---|
| uint16 | nWeights | 1–8 in the data [D]. |
| uint16 | replacement | m_iReplacement [B69]: the vertex this one collapses into at the next LOD. B69 says the runtime does not use it. The values do not all index the same LOD or LOD 0 [D], so it is kept as stored. |
| ×nWeights | uint32 node, float x, y, z, bias | NewVertexWeight [B69]: `m_iNode` and `m_Vec[4]`. |
| vec3 | location | Model space, at the bind pose. |
| vec3 | normal | |

**Skinning:** location = Σ over the weights of `nodes[node].bind_matrix · (x, y, z, bias)`.

- The weight's xyz is therefore already scaled by its bias.
- This holds within 0.1% for every vertex whose biases sum to 1: 428,715
  vertices, plus 373 that miss [D].
- 17,309 vertices in 214 files have biases that do not sum to 1. Examples are
  a single weight of 0.95, or two weights of 1.0 on the same node. For those,
  the stored location is authoritative.

### Nodes

The skeleton is written depth-first. Each node is followed by its children
[R `sub_1001bd54`, recursive]:

| Type | Field | Notes |
|---|---|---|
| string | name | |
| uint16 | index | Equal to the node's depth-first position in all 10,318 nodes [D]. |
| uint8 | flags | `MNODE_REMOVABLE` = 1, `MNODE_ROTATIONONLY` = 2 [B69]. The data has 0, 1 and 2 [D]. |
| mat4 | bind_matrix | The node's model-space transform at the bind pose. The engine keeps it and its inverse [R: `m_Mat2c = mat; m_Mat6c = ~mat`]. |
| uint32 | nChildren | |

After the tree come the weight sets [R `sub_1001c560`]:

- `uint32 n`
- for each set: `string name`, `uint32 count`, then `float × count`

R reads each weight as a dword. The count equals the number of nodes in all
16,138 sets [D].

### ChildModels

- `uint32 n`: from 1 to 16; the engine refuses 0 or more than 16 [R].
- Then n entries.
- Entry 0 is the model itself ("SELF" in R) and has no name. Entries 1 and up
  start with `string name`, a file name such as `pbase_Ps1.abc`.

Each entry then holds:

- `uint32 save_index`: m_SaveIndex [B69]. This is the child's save count when it
  was added. The engine compares it with the child's own entry 0 and treats the
  child's animation dims as stale on a mismatch [R `sub_1001c36c`; B69 comment].
- `(vec3 position, quat rotation)` × the number of nodes in **this** model:
  m_Relation [B69]. The relation is identity for entry 0 in 712 of 754 files,
  and for 337 of 350 other entries [D].

### Animation

- `uint32 n`, then n animations.

An animation [R `sub_1001bec9`, `sub_1001bc69`, `sub_1001bb37`]:

| Type | Field | Notes |
|---|---|---|
| vec3 | dims | The object's box half-sizes. A NOLF character is about (24, 53, 24). |
| string | name | |
| int32 | modeledit_weight_set | **Version ≥ 11 only.** m_ModelEditWeightSet [B69]. −1 in 3,300 animations, otherwise a weight-set index [D]. |
| uint32 | interpolation_time | **Version ≥ 12 only.** m_InterpolationMS [B69], in milliseconds. 200 is the default (the engine's constructor sets 0xC8 [R]). |
| uint32 | nKeyframes | |
| ×nKeyframes | uint32 time (ms), string command | The frame string. |
| ×nodes × nKeyframes | vec3 position, quat rotation | One block per node, in depth-first order, with all of that node's keyframes in each block. The block is a 28-byte CModelXform [R]. |

The transforms are **relative to the parent node**. Composing them down the
tree at the first key of the base animation reproduces the bind matrices, to
1.4e-4 on NOLF's Baroness (`pose_matrices`) [D].

### Sockets

- `uint32 n`, then n sockets [R `LoadSockets`].
- Each socket: `uint32 node`, `string name`, `quat rotation`, `vec3 position`.
- The node must be below the node count [R].

### AnimBindings

For each child-model entry, in ChildModels order [R `sub_1001c36c`]:

- `uint32 n`
- then n × (`string anim name`, `vec3 dims`, `vec3 translation`)

For entry 0, the names and dims equal the model's own animations in all 754
files. The translation is zero in 516 of them [D].

## Conventions for importers

- **Axes:** left-handed, Y up, as the engine renders. NOLF characters stand
  about 102 units tall, centred on the origin [D: Baroness].
- **Blender (right-handed, Z up):** mapping engine (x, y, z) to Blender
  (x, z, y) is a reflection, so that one swap fixes both the up axis and the
  handedness.
  - Matrices become P·M·P, with P the y/z swap.
  - A quaternion (x, y, z, w) becomes (−x, −z, −y, w).
  - The reflection reverses the apparent winding. Whether triangles need
    reordering on top of that depends on the engine's cull order, which is
    unverified here. Check it visually in the Blender layer.

  None of this is stored in the file; it is the Blender layer's job.
- **UVs:** v grows downward, as in DTX. Use 1 − v for Blender.

## Open questions

- The 62 header bytes the engine skips are zero in every file. ModelEdit's
  writer was not decompiled, so whatever it puts there is unconfirmed.
- 17,309 vertices have biases that do not sum to 1, and 373 vertices miss the
  skinning rule although their biases do sum to 1. Whether ModelEdit
  renormalises these, or the engine skins with the raw values, is untested.
- What `replacement` indexes is unclear. The values fit neither the vertex's
  own LOD nor LOD 0 consistently.
- Retail AvP2's models were not found. The four retail `.rez` files read
  (ALIEN, MARINE, PREDATOR, MULTI) hold no `.abc`, so they are presumably
  installed elsewhere from the disc. The Primal Hunt files cover v12 for AvP2.
- Versions before 9 (Shogo, Blood 2) and the `.ltb` generation (NOLF2 v23)
  belong in their own modules beside `abc.py`.
