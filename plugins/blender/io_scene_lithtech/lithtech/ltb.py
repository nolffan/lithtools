"""LithTech .ltb models (NOLF2, Contract J.A.C.K., Tron 2.0).

    from lithtech import ltb
    model = ltb.read_file("CATE.ltb")     # or ltb.read(data)
    data = ltb.write(model)                # bytes, identical to the input

See plugins/blender/docs/LTB.md for the layout and the evidence for each
field. Standard library only.

Two layouts exist:

* **D3D models** (LTB file type 1, LTB version 9; model file versions 23 to
  25): every model the games load. The engine reads them in
  Model::Load (model_load.cpp) and the D3D render objects' Load
  (d3dmeshrendobj_*.cpp); Model_Packer (lta2ltb_d3d.cpp) writes them.
* **Chunked** (a "LTBHeader" chunk, then chunks like .abc's): 31 leftover
  files in NOLF2's and Contract J.A.C.K.'s FX folders from before the
  format settled. The shipped engine refuses them (its first byte is
  read as the file type, 9). They are read as named chunks and written
  back as they are (ChunkedModel).

Conventions, as stored: left-handed, Y up; quaternions (x, y, z, w);
matrices row-major with the translation in the last column; times in ms.
"""

import struct
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

from .binio import Reader, Writer

# LTB_Header (ltb.h): uint8 type, (pad), uint16 version, uint8 reserved,
# (pad), uint32 x3: 20 bytes as MSVC lays the struct out.
LTB_HEADER_SIZE = 20
LTB_D3D_MODEL_FILE = 1
LTB_D3D_VERSION = 9            # CD3D_LTB_LOAD_VERSION
SUPPORTED_FILE_VERSIONS = (18, 20, 21, 22, 23, 24, 25)


class Layout:
    """What a model file version stores (see LTB.md). 23 to 25 are the
    shipped engine's (MIN_MODEL_FILE_VERSION .. MODEL_FILE_VERSION); 18 to
    22 are older packer output that NOLF2, Contract J.A.C.K. and Tron 2.0
    still ship, worked out from the files."""

    def __init__(self, version, **over):
        v = version
        # Found by which layout reads each version's files to the end and
        # writes them back unchanged (tests/roundtrip_ltb.py); no file of
        # version 19 exists.
        self.alloc_count = 15 if v >= 23 else 14       # m_nAnimData from 23
        self.render_priority = v >= 20                  # LOD m_nRenderPriority
        self.used_nodes = v >= 22                       # LOD used-node list
        # ChildModels as in .abc (a save index and a (position, rotation)
        # per node for every entry), version 18.
        self.child_relations = v < 20
        # REL compression: each kept key stored with its time (uint32 ms)
        # before its value, versions 18 and 20.
        self.rel_key_times = v <= 20
        # REL compression, versions 21 and 22: one dword after a non-zero
        # key count (always 0).
        self.rel_count_extra = v in (21, 22)
        # OBB records: version 18 (position, size, rotation quaternion,
        # node); ModelOBB_Depricated (position, size, 3x3 basis, node) to
        # 23; ModelOBB adds a radius from 24 (MIN_MODELOBB_FILE_VERSION).
        self.obb_format = "10fI" if v < 20 else ("15fI" if v < 24 else "15fIf")
        # Uncompressed keys: version 18 interleaves (position, rotation) per
        # key; later versions store the positions, then the rotations.
        self.none_interleaved = v < 20
        for k, val in over.items():
            setattr(self, k, val)


def layout(version, **over):
    return Layout(version, **over)
MAX_PIECE_TEXTURES = 4

# CRenderObject::RENDER_OBJECT_TYPES
RIGID_MESH = 4
SKEL_MESH = 5
VA_MESH = 6
NULL_MESH = 7

# Vertex data type flags (d3d_utils.h, "should match those in the LTB
# header").
VDT_POSITION = 0x0001
VDT_NORMAL = 0x0002
VDT_DIFFUSE = 0x0004
VDT_PSIZE = 0x0008
VDT_UVSETS_1 = 0x0010
VDT_UVSETS_2 = 0x0020
VDT_UVSETS_3 = 0x0040
VDT_UVSETS_4 = 0x0080
VDT_BASISVECTORS = 0x0100

# MODEL_ANIM_COMPRESSION_TYPES
ANCMPRS_NONE = 0
ANCMPRS_REL = 1
ANCMPRS_REL_16 = 2
ANCMPRS_REL_16_ROT_ONLY = 3

POS16_SCALE = 16.0             # NKF_TRANS_SCALE_1_11_4
QUAT16_SCALE = 0x7FFF


class LTBError(Exception):
    pass


# --------------------------------------------------------------------- #
#  Meshes
# --------------------------------------------------------------------- #

@dataclass
class Vertex:
    """One vertex as the D3D vertex streams hold it. Fields a stream
    doesn't carry stay None."""
    position: Optional[Tuple[float, float, float]] = None
    # The D3D blend weights ("betas"): one fewer than the bones blended.
    weights: Tuple[float, ...] = ()
    # Matrix-palette meshes: the four bone index bytes (D3DFVF_LASTBETA_UBYTE4).
    bone_indices: Optional[bytes] = None
    normal: Optional[Tuple[float, float, float]] = None
    uvs: List[Tuple[float, float]] = field(default_factory=list)   # per UV set; v down
    basis: Optional[Tuple[float, ...]] = None                     # 6 floats (tangent, binormal)


@dataclass
class BoneSet:
    """BoneSetListItem: a run of vertices skinned by up to four nodes."""
    first_vertex: int
    vertex_count: int
    bones: bytes                     # 4 node indices, 0xFF unused
    index_into_index_buffer: int


@dataclass
class RenderObject:
    """A LOD's geometry: CD3DRigidMesh, CD3DSkelMesh, CD3DVAMesh or a null
    mesh (CDIModelDrawable, no geometry). The fields that don't apply to a
    kind keep their defaults."""
    kind: int
    vertex_count: int = 0
    poly_count: int = 0
    max_bones_per_tri: int = 1
    max_bones_per_vert: int = 1
    stream_flags: List[int] = field(default_factory=lambda: [0, 0, 0, 0])
    bone_effector: int = 0           # rigid, VA: the node the mesh follows
    reindexed_bones_flag: int = 0    # skel: bool byte
    use_matrix_palettes: int = 0     # skel: bool byte
    min_bone: int = 0                # skel MP
    max_bone: int = 0
    reindexed_bones: List[int] = field(default_factory=list)
    undup_vertex_count: int = 0      # VA
    anim_node_index: int = 0         # VA
    vertices: List[Vertex] = field(default_factory=list)
    indices: List[int] = field(default_factory=list)       # 3 per triangle
    bone_sets: List[BoneSet] = field(default_factory=list)  # skel, not MP
    dup_map: List[Tuple[int, int]] = field(default_factory=list)  # VA: (src, dst)
    raw: bytes = b""                 # null mesh body (always empty in shipped files)
    # Floats per vertex for VDT_BASISVECTORS: 6 by d3d_utils.h; Build 69's
    # sphere_basis sample has 3.
    basis_floats: int = 6
    # Vertex bytes kept as stored when their layout can't be known (a
    # skeletal mesh with 0 bones per triangle, which the engine refuses).
    raw_vertex_data: Optional[bytes] = None

    def blend_layout(self):
        """(weights per vertex, has bone index bytes) of the position
        stream, as GetVertexFlags_and_Size picks the vertex type."""
        if self.kind == SKEL_MESH:
            if self.use_matrix_palettes:
                if self.max_bones_per_vert not in (2, 3, 4):
                    raise LTBError("matrix palette mesh with %d bones per vertex" % self.max_bones_per_vert)
                return self.max_bones_per_vert - 1, True
            if self.max_bones_per_tri == 0:
                # Tron's RECTORSCRIPT_SHARDS: Load_RD refuses it (assert(0));
                # read as unblended so the file still round-trips.
                return 0, False
            if self.max_bones_per_tri not in (1, 2, 3, 4):
                raise LTBError("skeletal mesh with %d bones per triangle" % self.max_bones_per_tri)
            return self.max_bones_per_tri - 1, False
        return 0, False


def uv_sets(flags):
    if flags & VDT_UVSETS_1:
        return 1
    if flags & VDT_UVSETS_2:
        return 2
    if flags & VDT_UVSETS_3:
        return 3
    if flags & VDT_UVSETS_4:
        return 4
    return 0


def vertex_size(flags, nweights, has_index, basis_floats=6):
    n = 0
    if flags & VDT_POSITION and flags & VDT_NORMAL:
        n += 12 + 4 * nweights + (4 if has_index else 0) + 12
    n += 8 * uv_sets(flags)
    if flags & VDT_BASISVECTORS:
        n += 4 * basis_floats
    return n


def _read_streams(r, ro):
    nw, has_index = ro.blend_layout()
    verts = [Vertex() for _ in range(ro.vertex_count)]
    for flags in ro.stream_flags:
        if not flags:
            continue
        pn = flags & VDT_POSITION and flags & VDT_NORMAL
        nuv = uv_sets(flags)
        basis = flags & VDT_BASISVECTORS
        for v in verts:
            if pn:
                v.position = r.vec3()
                v.weights = r.unpack("%df" % nw) if nw else ()
                if has_index:
                    v.bone_indices = r.raw(4)
                v.normal = r.vec3()
            if nuv:
                v.uvs = [r.unpack("2f") for _ in range(nuv)]
            if basis:
                v.basis = r.unpack("%df" % ro.basis_floats)
    ro.vertices = verts


def _write_streams(w, ro):
    nw, has_index = ro.blend_layout()
    for flags in ro.stream_flags:
        if not flags:
            continue
        pn = flags & VDT_POSITION and flags & VDT_NORMAL
        nuv = uv_sets(flags)
        basis = flags & VDT_BASISVECTORS
        for v in ro.vertices:
            if pn:
                w.vec3(v.position)
                if nw:
                    w.pack("%df" % nw, *v.weights)
                if has_index:
                    w.raw(v.bone_indices)
                w.vec3(v.normal)
            if nuv:
                for i in range(nuv):
                    w.pack("2f", *v.uvs[i])
            if basis:
                w.pack("%df" % len(v.basis), *v.basis)


def _read_render_object(r, kind):
    start = r.pos
    try:
        return _read_render_object_as(r, kind, 6)
    except (LTBError, EOFError, struct.error):
        # A basis-vector mesh whose vertices hold 3 basis floats.
        r.pos = start
        return _read_render_object_as(r, kind, 3)


def _read_render_object_as(r, kind, basis_floats):
    size = r.u32()
    start = r.pos
    ro = RenderObject(kind, basis_floats=basis_floats)
    if kind == NULL_MESH:
        ro.raw = r.raw(size)
        return ro
    if kind not in (RIGID_MESH, SKEL_MESH, VA_MESH):
        raise LTBError("unknown render object type %d" % kind)
    ro.vertex_count = r.u32()
    if kind == VA_MESH:
        ro.undup_vertex_count = r.u32()
    ro.poly_count = r.u32()
    ro.max_bones_per_tri = r.u32()
    ro.max_bones_per_vert = r.u32()
    if kind == SKEL_MESH:
        ro.reindexed_bones_flag = r.u8()
    ro.stream_flags = list(r.unpack("4I"))
    if kind == SKEL_MESH:
        ro.use_matrix_palettes = r.u8()
        if ro.use_matrix_palettes:
            ro.min_bone = r.u32()
            ro.max_bone = r.u32()
            if ro.reindexed_bones_flag:
                n = r.u32()
                ro.reindexed_bones = list(r.unpack("%dI" % n)) if n else []
    elif kind == VA_MESH:
        ro.anim_node_index = r.u32()
        ro.bone_effector = r.u32()
    else:
        ro.bone_effector = r.u32()
    if kind == SKEL_MESH and not ro.use_matrix_palettes and ro.max_bones_per_tri == 0:
        # Find the vertex size that leaves exactly the indices and a whole
        # number of bone sets.
        rest = size - (r.pos - start)
        for vsize in range(4, 129, 4):
            vbytes = vsize * ro.vertex_count
            tail = rest - vbytes - 6 * ro.poly_count
            if tail < 4:
                break
            at = r.pos + vbytes + 6 * ro.poly_count
            nsets = int.from_bytes(r.data[at:at + 4], "little")
            if tail == 4 + 12 * nsets:
                ro.raw_vertex_data = r.raw(vbytes)
                break
        else:
            raise LTBError("can't find the vertex size of a 0-bone skeletal mesh")
        if ro.raw_vertex_data is None:
            raise LTBError("can't find the vertex size of a 0-bone skeletal mesh")
    else:
        _read_streams(r, ro)
    n = ro.poly_count * 3
    ro.indices = list(r.unpack("%dH" % n)) if n else []
    if kind == SKEL_MESH and not ro.use_matrix_palettes:
        for _ in range(r.u32()):
            first, count = r.unpack("HH")
            bones = r.raw(4)
            ro.bone_sets.append(BoneSet(first, count, bones, r.u32()))
    elif kind == VA_MESH:
        for _ in range(r.u32()):
            ro.dup_map.append(r.unpack("HH"))
    if r.pos - start != size:
        raise LTBError("render object: read %d bytes of %d" % (r.pos - start, size))
    return ro


def _write_render_object(w, ro):
    at = w.tell()
    w.u32(0)
    if ro.kind == NULL_MESH:
        w.raw(ro.raw)
    else:
        w.u32(ro.vertex_count if ro.raw_vertex_data is not None or not ro.vertices else len(ro.vertices))
        if ro.kind == VA_MESH:
            w.u32(ro.undup_vertex_count)
        w.u32(len(ro.indices) // 3)
        w.u32(ro.max_bones_per_tri)
        w.u32(ro.max_bones_per_vert)
        if ro.kind == SKEL_MESH:
            w.u8(ro.reindexed_bones_flag)
        w.pack("4I", *ro.stream_flags)
        if ro.kind == SKEL_MESH:
            w.u8(ro.use_matrix_palettes)
            if ro.use_matrix_palettes:
                w.u32(ro.min_bone)
                w.u32(ro.max_bone)
                if ro.reindexed_bones_flag:
                    w.u32(len(ro.reindexed_bones))
                    if ro.reindexed_bones:
                        w.pack("%dI" % len(ro.reindexed_bones), *ro.reindexed_bones)
        elif ro.kind == VA_MESH:
            w.u32(ro.anim_node_index)
            w.u32(ro.bone_effector)
        else:
            w.u32(ro.bone_effector)
        if ro.raw_vertex_data is not None:
            w.raw(ro.raw_vertex_data)
        else:
            _write_streams(w, ro)
        if ro.indices:
            w.pack("%dH" % len(ro.indices), *ro.indices)
        if ro.kind == SKEL_MESH and not ro.use_matrix_palettes:
            w.u32(len(ro.bone_sets))
            for b in ro.bone_sets:
                w.pack("HH", b.first_vertex, b.vertex_count)
                w.raw(b.bones)
                w.u32(b.index_into_index_buffer)
        elif ro.kind == VA_MESH:
            w.u32(len(ro.dup_map))
            for s, d in ro.dup_map:
                w.pack("HH", s, d)
    w.patch_u32(at, w.tell() - at - 4)


# --------------------------------------------------------------------- #
#  Model
# --------------------------------------------------------------------- #

@dataclass
class LOD:
    texture_count: int = 0
    textures: List[int] = field(default_factory=lambda: [-1, -1, -1, -1])  # skin slots
    render_style: int = 0
    render_priority: int = 0
    mesh: RenderObject = field(default_factory=lambda: RenderObject(NULL_MESH))
    used_nodes: bytes = b""          # the nodes this LOD uses (CalcUsedNodes recomputes it)


@dataclass
class Piece:
    name: str
    lod_distances: List[float] = field(default_factory=list)   # one per LOD
    # "min/max lod offsets, no longer used" (model_load.cpp)
    min_lod_offset: int = 0
    max_lod_offset: int = 0
    lods: List[LOD] = field(default_factory=list)


@dataclass
class Node:
    name: str
    index: int                       # uint16
    flags: int                       # uint8
    bind_matrix: Tuple[float, ...]   # 16 floats: the node's global transform at bind
    children: List["Node"] = field(default_factory=list)


@dataclass
class WeightSet:
    name: str
    weights: List[float]


@dataclass
class Keyframe:
    time: int
    command: str


@dataclass
class NodeTrack:
    """One node's keys in one animation, as stored.

    For ANCMPRS_NONE: is_vertex_anim, then either vertex_frames (a list of
    float triples per keyframe) or positions/rotations, one per keyframe.
    For the REL kinds: positions and rotations hold only the keys kept (0,
    1 or one per keyframe); ints for the 16-bit encodings (positions *
    POS16_SCALE, quaternion parts * 0x7FFF)."""
    positions: List[tuple] = field(default_factory=list)
    rotations: List[tuple] = field(default_factory=list)
    is_vertex_anim: int = 0
    vertex_frames: List[List[float]] = field(default_factory=list)
    # Version 18 (REL kinds): the time (ms) of each kept key.
    pos_times: List[int] = field(default_factory=list)
    rot_times: List[int] = field(default_factory=list)
    # Versions 19 to 22 (REL kinds): the dword after a non-zero count.
    pos_extra: int = 0
    rot_extra: int = 0


@dataclass
class Animation:
    name: str
    dims: Tuple[float, float, float]
    compression: int = ANCMPRS_NONE
    interpolation_time: int = 200
    keyframes: List[Keyframe] = field(default_factory=list)
    tracks: List[NodeTrack] = field(default_factory=list)   # Model.nodes order

    def key_transform(self, node, key):
        """A node's (position, rotation) at a keyframe, decoded as the
        engine's channel interpreters do: no keys is identity, one key holds
        for every keyframe, otherwise one key per keyframe. Versions 18 and
        20 store each kept key with its time; between them the value is
        interpolated linearly (normalised for rotations)."""
        t = self.tracks[node]
        c = self.compression
        if c == ANCMPRS_NONE:
            if t.is_vertex_anim:
                return (0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)
            return tuple(t.positions[key]), tuple(t.rotations[key])
        time = self.keyframes[key].time if self.keyframes else 0
        # the nth keyframe at a time takes the nth key at that time
        rank = sum(1 for k in self.keyframes[:key] if k.time == time)
        pos = _channel(t.positions, t.pos_times, key, time, (0.0, 0.0, 0.0), rank)
        rot = _channel(t.rotations, t.rot_times, key, time, (0.0, 0.0, 0.0, 1.0), rank)
        if c == ANCMPRS_REL_16 and t.positions:
            pos = tuple(x / POS16_SCALE for x in pos)
        if c in (ANCMPRS_REL_16, ANCMPRS_REL_16_ROT_ONLY) and t.rotations:
            rot = tuple(x / QUAT16_SCALE for x in rot)
            n = sum(x * x for x in rot) ** 0.5 or 1.0
            rot = tuple(x / n for x in rot)
        return tuple(pos), tuple(rot)


def _channel(values, times, key, time, identity, rank=0):
    if not values:
        return identity
    if len(values) == 1:
        return values[0]
    if not times:
        return values[key]
    exact = [i for i, t in enumerate(times) if t == time]
    if exact:
        return values[exact[min(rank, len(exact) - 1)]]
    if time <= times[0]:
        return values[0]
    for i in range(1, len(times)):
        if time <= times[i]:
            t0, t1 = times[i - 1], times[i]
            f = (time - t0) / (t1 - t0) if t1 > t0 else 0.0
            a, b = values[i - 1], values[i]
            if len(a) == 4 and sum(x * y for x, y in zip(a, b)) < 0:
                b = tuple(-x for x in b)
            return tuple(x + (y - x) * f for x, y in zip(a, b))
    return values[-1]


@dataclass
class Socket:
    node: int
    name: str
    rotation: Tuple[float, float, float, float]
    position: Tuple[float, float, float]
    scale: Tuple[float, float, float] = (1.0, 1.0, 1.0)


@dataclass
class AnimBinding:
    name: str
    dims: Tuple[float, float, float]
    translation: Tuple[float, float, float]


@dataclass
class Model:
    header: bytes = bytes([LTB_D3D_MODEL_FILE, 0, LTB_D3D_VERSION, 0]) + bytes(16)
    version: int = 23
    # ModelAllocations, as stored; None makes write() compute them.
    alloc_counts: Optional[List[int]] = None
    command_string: str = ""
    radius: float = 96.0             # m_VisRadius
    obbs: List[tuple] = field(default_factory=list)  # flat, as Layout.obb_format
    pieces: List[Piece] = field(default_factory=list)
    root: Optional[Node] = None
    weight_sets: List[WeightSet] = field(default_factory=list)
    child_models: List[str] = field(default_factory=list)  # file names, not counting self
    # Old files (version 18): per child entry, self first, (save index,
    # [(position, rotation) per node]).
    child_relations: List[tuple] = field(default_factory=list)
    animations: List[Animation] = field(default_factory=list)
    sockets: List[Socket] = field(default_factory=list)
    anim_bindings: List[List[AnimBinding]] = field(default_factory=list)  # per child model, self first

    @property
    def file_type(self):
        return self.header[0]

    @property
    def ltb_version(self):
        return self.header[2] | self.header[3] << 8

    @property
    def nodes(self) -> List[Node]:
        out = []
        if self.root is not None:
            stack = [self.root]
            while stack:
                n = stack.pop()
                out.append(n)
                stack.extend(reversed(n.children))
        return out

    def node_parents(self) -> List[int]:
        nodes = self.nodes
        idx = {id(n): i for i, n in enumerate(nodes)}
        parents = [-1] * len(nodes)
        for n in nodes:
            for c in n.children:
                parents[idx[id(c)]] = idx[id(n)]
        return parents


def quat_to_matrix(position, rotation):
    x, y, z, w = rotation
    return (1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), position[0],
            2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), position[1],
            2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), position[2],
            0.0, 0.0, 0.0, 1.0)


def matrix_multiply(a, b):
    return tuple(sum(a[4 * i + k] * b[4 * k + j] for k in range(4)) for i in range(4) for j in range(4))


def pose_matrices(m, anim, key):
    """Each node's model-space matrix at a keyframe: the parent-relative
    transforms composed down the tree."""
    parents = m.node_parents()
    out = []
    for i in range(len(m.nodes)):
        local = quat_to_matrix(*anim.key_transform(i, key))
        out.append(local if parents[i] < 0 else matrix_multiply(out[parents[i]], local))
    return out


ALLOC_NAMES = ("keyframes", "parent_anims", "nodes", "pieces", "child_models", "tris", "verts",
               "vertex_weights", "lods", "sockets", "weight_sets", "strings", "string_lengths",
               "vert_anim_data_size", "anim_data")


# The allocation counts that size the engine's block allocator and have
# to be exact (keyframes, animations, nodes, child models, sockets, vertex
# animation data, animation data); weight sets too, except in 8 files whose
# packer counted sets it then dropped.
ALLOC_EXACT = (0, 1, 2, 4, 9, 13, 14)


def track_data_size(anim, track):
    """Bytes of channel data the engine keeps for a track (AnimNode's
    Setup*Channel), toward ModelAllocations::m_nAnimData."""
    c = anim.compression
    if c == ANCMPRS_NONE:
        if track.is_vertex_anim:
            return 0
        return 12 * len(track.positions) + 16 * len(track.rotations)
    ps = 6 if c == ANCMPRS_REL_16 else 12
    qs = 8 if c in (ANCMPRS_REL_16, ANCMPRS_REL_16_ROT_ONLY) else 16
    return ps * len(track.positions) + qs * len(track.rotations)


def compute_alloc_counts(m: Model) -> List[int]:
    """ModelAllocations as Model_Packer fills them, as far as the files
    show (see LTB.md); vert_anim_data_size and the vertex/weight counts are
    what the packer measured on its own data."""
    tris = verts = 0
    lods = 0
    for p in m.pieces:
        for lod in p.lods:
            lods += 1
            tris += len(lod.mesh.indices) // 3
            verts += len(lod.mesh.vertices)
    strings = set()
    for s in [m.command_string] + [n.name for n in m.nodes] + [p.name for p in m.pieces] + \
             m.child_models + [a.name for a in m.animations] + \
             [k.command for a in m.animations for k in a.keyframes] + \
             [s.name for s in m.sockets] + [w.name for w in m.weight_sets]:
        if s:
            strings.add(s)
    anim_data = sum(track_data_size(a, t) for a in m.animations for t in a.tracks)
    vad = 0
    for a in m.animations:
        for t in a.tracks:
            for fr in t.vertex_frames:
                vad += 4 * len(fr)
    return [sum(len(a.keyframes) for a in m.animations), len(m.animations), len(m.nodes),
            len(m.pieces), 1 + len(m.child_models), tris, verts, verts, lods, len(m.sockets),
            len(m.weight_sets), len(strings), sum(len(s) for s in strings), vad, anim_data]


def _read_node(r):
    name = r.string()
    index = r.u16()
    flags = r.u8()
    mat = r.unpack("16f")
    node = Node(name, index, flags, mat)
    for _ in range(r.u32()):
        node.children.append(_read_node(r))
    return node


def _write_node(w, n):
    w.string(n.name)
    w.u16(n.index)
    w.u8(n.flags)
    w.pack("16f", *n.bind_matrix)
    w.u32(len(n.children))
    for c in n.children:
        _write_node(w, c)


def _read_track(r, anim, nkeys, L):
    t = NodeTrack()
    c = anim.compression
    if c == ANCMPRS_NONE:
        t.is_vertex_anim = r.u8()
        if t.is_vertex_anim:
            for _ in range(nkeys):
                n = r.u32()
                t.vertex_frames.append(list(r.unpack("%df" % (3 * n))) if n else [])
        elif L.none_interleaved:
            for _ in range(nkeys):
                t.positions.append(r.vec3())
                t.rotations.append(r.quat())
        else:
            t.positions = [r.vec3() for _ in range(nkeys)]
            t.rotations = [r.quat() for _ in range(nkeys)]
        return t
    if c not in (ANCMPRS_REL, ANCMPRS_REL_16, ANCMPRS_REL_16_ROT_ONLY):
        raise LTBError("unknown animation compression %d" % c)
    q16 = c in (ANCMPRS_REL_16, ANCMPRS_REL_16_ROT_ONLY)
    n = r.u32()
    if n and L.rel_count_extra:
        t.pos_extra = r.u32()
    for _ in range(n):
        if L.rel_key_times:
            t.pos_times.append(r.u32())
        t.positions.append(r.unpack("3h") if c == ANCMPRS_REL_16 else r.vec3())
    n = r.u32()
    if n and L.rel_count_extra:
        t.rot_extra = r.u32()
    for _ in range(n):
        if L.rel_key_times:
            t.rot_times.append(r.u32())
        t.rotations.append(r.unpack("4h") if q16 else r.quat())
    return t


def _write_track(w, anim, t, L):
    c = anim.compression
    if c == ANCMPRS_NONE:
        w.u8(t.is_vertex_anim)
        if t.is_vertex_anim:
            for fr in t.vertex_frames:
                w.u32(len(fr) // 3)
                if fr:
                    w.pack("%df" % len(fr), *fr)
        elif L.none_interleaved:
            for p, q in zip(t.positions, t.rotations):
                w.vec3(p)
                w.quat(q)
        else:
            for p in t.positions:
                w.vec3(p)
            for q in t.rotations:
                w.quat(q)
        return
    q16 = c in (ANCMPRS_REL_16, ANCMPRS_REL_16_ROT_ONLY)
    w.u32(len(t.positions))
    if t.positions and L.rel_count_extra:
        w.u32(t.pos_extra)
    for i, p in enumerate(t.positions):
        if L.rel_key_times:
            w.u32(t.pos_times[i])
        w.pack("3h", *p) if c == ANCMPRS_REL_16 else w.vec3(p)
    w.u32(len(t.rotations))
    if t.rotations and L.rel_count_extra:
        w.u32(t.rot_extra)
    for i, q in enumerate(t.rotations):
        if L.rel_key_times:
            w.u32(t.rot_times[i])
        w.pack("4h", *q) if q16 else w.quat(q)


_overrides = {}   # for working out old layouts (tests)
_stage = [""]     # the section being read, for error messages


def _read_d3d(data) -> Model:
    r = Reader(data, 0)
    m = Model()
    m.header = r.raw(LTB_HEADER_SIZE)
    if m.file_type != LTB_D3D_MODEL_FILE:
        raise LTBError("LTB file type %d is not a D3D model" % m.file_type)
    if m.ltb_version != LTB_D3D_VERSION:
        raise LTBError("LTB version %d (only %d is known)" % (m.ltb_version, LTB_D3D_VERSION))
    m.version = r.u32()
    if m.version not in SUPPORTED_FILE_VERSIONS:
        raise LTBError("model file version %d (18 to 25 are known)" % m.version)
    L = layout(m.version, **_overrides)
    _stage[0] = "header"
    m.alloc_counts = list(r.unpack("%dI" % L.alloc_count))
    m.command_string = r.string()
    m.radius = r.f32()
    for _ in range(r.u32()):
        m.obbs.append(r.unpack(L.obb_format))
    _stage[0] = "pieces"
    for _ in range(r.u32()):
        p = Piece(r.string())
        nlods = r.u32()
        p.lod_distances = list(r.unpack("%df" % nlods)) if nlods else []
        p.min_lod_offset, p.max_lod_offset = r.unpack("II")
        for _ in range(nlods):
            lod = LOD()
            lod.texture_count = r.u32()
            lod.textures = list(r.unpack("4i"))
            lod.render_style = r.i32()
            if L.render_priority:
                lod.render_priority = r.u8()
            kind = r.u32()
            lod.mesh = _read_render_object(r, kind)
            if L.used_nodes:
                n = r.u8()                   # m_UsedNodeListSize is a uint8
                lod.used_nodes = r.raw(n)
            p.lods.append(lod)
        m.pieces.append(p)
    _stage[0] = "nodes"
    m.root = _read_node(r)
    nodes = m.nodes
    for _ in range(r.u32()):
        name = r.string()
        n = r.u32()
        m.weight_sets.append(WeightSet(name, list(r.unpack("%df" % n)) if n else []))
    _stage[0] = "child models"
    nchild = r.u32()
    if nchild == 0:
        raise LTBError("no child models (the model itself counts as one)")
    if L.child_relations:
        for i in range(nchild):
            if i:
                m.child_models.append(r.string())
            save = r.u32()
            m.child_relations.append((save, [(r.vec3(), r.quat()) for _ in nodes]))
    else:
        m.child_models = [r.string() for _ in range(nchild - 1)]
    _stage[0] = "animations"
    for _ in range(r.u32()):
        dims = r.vec3()
        a = Animation(r.string(), dims)
        a.compression = r.u32()
        a.interpolation_time = r.u32()
        for _ in range(r.u32()):
            t = r.u32()
            a.keyframes.append(Keyframe(t, r.string()))
        a.tracks = [_read_track(r, a, len(a.keyframes), L) for _ in nodes]
        m.animations.append(a)
    _stage[0] = "sockets"
    for _ in range(r.u32()):
        node = r.u32()
        name = r.string()
        rot = r.quat()
        pos = r.vec3()
        m.sockets.append(Socket(node, name, rot, pos, r.vec3()))
    _stage[0] = "anim bindings"
    for _ in range(nchild):
        binds = []
        for _ in range(r.u32()):
            name = r.string()
            binds.append(AnimBinding(name, r.vec3(), r.vec3()))
        m.anim_bindings.append(binds)
    if r.pos != len(data):
        raise LTBError("%d bytes left after the model" % (len(data) - r.pos))
    return m


def _write_d3d(m: Model) -> bytes:
    w = Writer()
    w.raw(m.header)
    w.u32(m.version)
    L = layout(m.version, **_overrides)
    counts = m.alloc_counts if m.alloc_counts is not None else compute_alloc_counts(m)[:L.alloc_count]
    w.pack("%dI" % L.alloc_count, *counts)
    w.string(m.command_string)
    w.f32(m.radius)
    w.u32(len(m.obbs))
    for o in m.obbs:
        w.pack(L.obb_format, *o)
    w.u32(len(m.pieces))
    for p in m.pieces:
        w.string(p.name)
        w.u32(len(p.lods))
        if p.lods:
            w.pack("%df" % len(p.lods), *p.lod_distances)
        w.pack("II", p.min_lod_offset, p.max_lod_offset)
        for lod in p.lods:
            w.u32(lod.texture_count)
            w.pack("4i", *lod.textures)
            w.i32(lod.render_style)
            if L.render_priority:
                w.u8(lod.render_priority)
            w.u32(lod.mesh.kind)
            _write_render_object(w, lod.mesh)
            if L.used_nodes:
                w.u8(len(lod.used_nodes))
                w.raw(lod.used_nodes)
    _write_node(w, m.root)
    w.u32(len(m.weight_sets))
    for ws in m.weight_sets:
        w.string(ws.name)
        w.u32(len(ws.weights))
        if ws.weights:
            w.pack("%df" % len(ws.weights), *ws.weights)
    w.u32(1 + len(m.child_models))
    if L.child_relations:
        nn = len(m.nodes)
        ident = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))
        for i in range(1 + len(m.child_models)):
            if i:
                w.string(m.child_models[i - 1])
            save, rel = m.child_relations[i] if i < len(m.child_relations) else (0, [ident] * nn)
            w.u32(save)
            for p, q in rel:
                w.vec3(p)
                w.quat(q)
    else:
        for c in m.child_models:
            w.string(c)
    w.u32(len(m.animations))
    for a in m.animations:
        w.vec3(a.dims)
        w.string(a.name)
        w.u32(a.compression)
        w.u32(a.interpolation_time)
        w.u32(len(a.keyframes))
        for k in a.keyframes:
            w.u32(k.time)
            w.string(k.command)
        for t in a.tracks:
            _write_track(w, a, t, L)
    w.u32(len(m.sockets))
    for s in m.sockets:
        w.u32(s.node)
        w.string(s.name)
        w.quat(s.rotation)
        w.vec3(s.position)
        w.vec3(s.scale)
    binds = m.anim_bindings or [[]]
    for b in binds:
        w.u32(len(b))
        for ab in b:
            w.string(ab.name)
            w.vec3(ab.dims)
            w.vec3(ab.translation)
    return w.getvalue()


# --------------------------------------------------------------------- #
#  Chunked (pre-release) files
# --------------------------------------------------------------------- #

@dataclass
class ChunkedModel:
    """A chunked .ltb: (name, body) per chunk, in order; the next-chunk
    offsets are rebuilt on write. The engine never loads these."""
    chunks: List[Tuple[str, bytes]] = field(default_factory=list)

    @property
    def ltb_header(self):
        """(file type, LTB version) from the LTBHeader chunk."""
        for name, body in self.chunks:
            if name == "LTBHeader":
                return body[0], body[2] | body[3] << 8
        return None


def _read_chunked(data) -> ChunkedModel:
    m = ChunkedModel()
    pos = 0
    for _ in range(0x200):
        r = Reader(data, pos)
        name = r.string()
        nxt = r.u32()
        end = len(data) if nxt == 0xFFFFFFFF else nxt
        if end < r.pos or end > len(data):
            raise LTBError("bad chunk offset")
        m.chunks.append((name, bytes(data[r.pos:end])))
        if nxt == 0xFFFFFFFF:
            return m
        pos = nxt
    raise LTBError("chunk list does not end")


def _write_chunked(m: ChunkedModel) -> bytes:
    w = Writer()
    for i, (name, body) in enumerate(m.chunks):
        w.string(name)
        at = w.tell()
        w.u32(0)
        w.raw(body)
        w.patch_u32(at, 0xFFFFFFFF if i == len(m.chunks) - 1 else w.tell())
    return w.getvalue()


# --------------------------------------------------------------------- #
#  Entry points
# --------------------------------------------------------------------- #

def is_chunked(data):
    return bytes(data[:11]) == b"\x09\x00LTBHeader"


def read(data):
    """A Model (D3D) or a ChunkedModel."""
    if len(data) < LTB_HEADER_SIZE:
        raise LTBError("too short for an .ltb")
    if is_chunked(data):
        return _read_chunked(data)
    try:
        return _read_d3d(data)
    except (EOFError, ValueError, struct.error) as e:   # a short or damaged file
        raise LTBError("truncated or damaged .ltb (in %s): %s" % (_stage[0], e))


def read_file(path):
    with open(path, "rb") as f:
        return read(f.read())


def write(m) -> bytes:
    if isinstance(m, ChunkedModel):
        return _write_chunked(m)
    return _write_d3d(m)


def write_file(m, path):
    with open(path, "wb") as f:
        f.write(write(m))
