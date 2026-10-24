"""PlayStation 2 NOLF models: .ltb file type 2, version 16 (The Operative:
No One Lives Forever on the PS2).

    from lithtech import ps2ltb
    model = ps2ltb.read_file("hero_action.ltb")   # or ps2ltb.read(data)
    data = ps2ltb.write(model)                      # bytes, identical to the input

See plugins/blender/docs/PS2LTB.md for the layout. Every model on both PS2
discs (1,335 of them) reads and writes back byte for byte. Standard library
only, so it runs inside Blender's Python.

Geometry is what the PS2 sends its vector unit: per LOD a DMA chain of
buffers, each a header and triangle strips of (position, normal, s t q)
vertices. Rigid meshes hold their vertices in their node's space; skeletal
meshes in model space, with a list of unique vertices (positions, normals,
up to four weights) and a map from each unique vertex to its copies in the
strips. `Strip.triangles()` gives a strip's triangles in PC (clockwise)
order.

Piece, animation, weight-set and socket names are stored only as hashes
(with a multiplier each file chooses); `Names` turns them back into names
from a dictionary (ps2ltb_names.txt beside this module, plus any names
added). A hash with no name shows as "#xxxxxxxx" and stays that hash.

Conventions as in abc.py: left-handed, Y up; quaternions (x, y, z, w);
matrices 16 floats row-major with the translation in the last column;
times in milliseconds.
"""

import os
import struct
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

from .binio import Reader, Writer

FILE_TYPE = 2
VERSION = 16
RIGID_MESH, SKEL_MESH = 4, 5

# VIF codes and DMA tag ids the chains use.
VIF_NOP, VIF_MSCALF, VIF_FLUSH, VIF_UNPACK_V4_32 = 0x00, 0x15, 0x11, 0x6C
DMA_CNT, DMA_RET = 1, 6
VU_BASE = 80              # where each buffer goes in VU memory
MAX_UNPACK = 255          # qwords per UNPACK, as the files have them
MAX_BUFFER = 844          # qwords per buffer (the largest in any shipped file)

# GIF tag words of a strip, as every shipped file has them (lo without
# NLOOP/EOP: PRE, PRIM 0x5c textured Gouraud triangle strip, packed, 3
# registers; hi: REGS ST, RGBAQ, XYZF2; 0x8000 marks a strip whose first
# triangle is reversed).
GIF_LO_REST = 0x302E400000000000
GIF_REGS = 0x412
GIF_REVERSED = 0x8000


class PS2LTBError(Exception):
    pass


def name_hash(name: str, mult: int) -> int:
    """The hash a PS2 model stores for a name: the archive's own (upper case,
    '/' as '\\'), h = h + c + h * mult."""
    h = 0
    for ch in name.upper():
        c = ord(ch)
        if c == 0x2F:
            c = 0x5C
        h = (h + c + h * mult) & 0xFFFFFFFF
    return h


# --------------------------------------------------------------------- #
#  Names

# The names the game code asks models for, first: with a small multiplier
# names collide (with 2, "Report" and "Select" hash the same), and the one
# the code uses wins (the nolfsrc ps2 branch's ps2model.PRIORITY).
PRIORITY = (["Select", "Deselect", "Reload", "Fire", "AltFire", "Animation", "Select1", "Dec1"] +
            ["Idle_%d" % i for i in range(10)] + ["AltIdle_%d" % i for i in range(10)] +
            ["Fire%d" % i for i in range(10)] + ["AltFire%d" % i for i in range(10)])

UNNAMED = "#%08x"


class Names:
    """Name lookup for hashes, per multiplier. Earlier names win a collision."""

    def __init__(self, names=None, path=None):
        if names is None:
            names = []
            path = path or os.path.join(os.path.dirname(os.path.abspath(__file__)), "ps2ltb_names.txt")
            if os.path.isfile(path):
                with open(path, encoding="latin-1") as fh:
                    names = [l.rstrip("\r\n") for l in fh if l.strip() and not l.startswith("#")]
        self.names = list(PRIORITY) + list(names)
        self._tables = {}

    def add(self, names, first=True):
        self.names = (list(names) + self.names) if first else (self.names + list(names))
        self._tables = {}

    def lookup(self, h: int, mult: int) -> Optional[str]:
        t = self._tables.get(mult)
        if t is None:
            t = {}
            for n in reversed(self.names):
                t[name_hash(n, mult)] = n
            self._tables[mult] = t
        return t.get(h)

    def name(self, h: int, mult: int) -> str:
        n = self.lookup(h, mult)
        return n if n is not None else UNNAMED % h


_default_names = None


def default_names() -> Names:
    global _default_names
    if _default_names is None:
        _default_names = Names()
    return _default_names


def hash_of(name: str, mult: int) -> int:
    """A name's hash; "#xxxxxxxx" (an unresolved hash) gives back that hash."""
    if len(name) == 9 and name[0] == "#":
        try:
            return int(name[1:], 16)
        except ValueError:
            pass
    return name_hash(name, mult)


# --------------------------------------------------------------------- #
#  Data

Vec3 = Tuple[float, float, float]


@dataclass
class StripVertex:
    position: Tuple[float, float, float, float]   # x y z 1 (rigid: node space; skeletal: model space)
    normal: Tuple[float, float, float, float]     # nx ny nz 0
    stq: Tuple[float, float, float]               # s t q (q = 1)
    extra: int = 0                                # the u32 after q; meaning unknown (mostly 0)
    unique: int = -1                              # skeletal: the unique vertex it copies

    @property
    def uv(self):
        s, t, q = self.stq
        return (s / q, t / q) if q else (s, t)


@dataclass
class Strip:
    vertices: List[StripVertex] = field(default_factory=list)
    reversed: bool = False       # the first triangle goes the other way
    gif_lo_rest: int = GIF_LO_REST
    gif_hi: int = GIF_REGS       # REGS (with GIF_REVERSED) and an upper u32 (unknown, kept)

    def triangles(self):
        """(i, j, k) indices into `vertices`, PC (clockwise) order."""
        out = []
        for k in range(2, len(self.vertices)):
            a, b, c = k - 2, k - 1, k
            if (k % 2 == 1) != self.reversed:
                a, b = b, a
            out.append((a, b, c))
        return out


@dataclass
class Buffer:
    """One VU buffer (one DMA tag, one MSCALF): a header and strips."""
    strips: List[Strip] = field(default_factory=list)
    header_rest: bytes = b"\0" * 8   # the header qword after strips-1 and the vertex count


@dataclass
class UniqueVertex:
    position: Tuple[float, float, float, float]   # x y z 1, model space
    normal: Tuple[float, float, float, float]     # nx ny nz 0
    weights: Tuple[int, int, int, int] = (4096, 0, 0, 0)   # s16, 4096 = 1.0
    bones: Tuple[int, int, int, int] = (0, 0xFF, 0xFF, 0xFF)   # local bone index * 4, 0xff unused

    def node_weights(self, bone_map):
        """[(node, weight)] as the file has them (weights / 4096)."""
        out = []
        for w, b in zip(self.weights, self.bones):
            if b != 0xFF and w:
                out.append((bone_map[b // 4], w / 4096.0))
        return out


@dataclass
class LOD:
    kind: int = RIGID_MESH
    node: int = 0                    # rigid: the node the mesh hangs on
    buffers: List[Buffer] = field(default_factory=list)
    # skeletal
    uniques: List[UniqueVertex] = field(default_factory=list)
    bone_map: List[int] = field(default_factory=list)   # local bone -> node
    wmap_pad: bytes = b""            # the weight map's padding to 16 bytes
    # as stored (for checking): chain qwords, weight map entries
    stored_qwords: int = 0
    stored_wmap_entries: int = 0

    def strips(self):
        return [s for b in self.buffers for s in b.strips]

    def triangle_count(self):
        return sum(max(len(s.vertices) - 2, 0) for s in self.strips())


@dataclass
class Piece:
    name_hash: int = 0
    colors: Tuple[float, ...] = (0.8, 0.8, 0.8, 1.0, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0)
    texture: int = 0
    unknown: Tuple[int, int, int] = (0, 0, 4)
    lods: List[LOD] = field(default_factory=list)


@dataclass
class Node:
    name: str
    bind_matrix: Tuple[float, ...]
    name_hash: int = 0
    child_count: int = 0
    index: int = 0
    flags: int = 0


@dataclass
class WeightSet:
    name_hash: int
    weights: List[float]


@dataclass
class Keyframe:
    time: int
    command: str = ""


@dataclass
class Animation:
    dims: Vec3 = (0.0, 0.0, 0.0)
    translation: Vec3 = (0.0, 0.0, 0.0)
    name_hash: int = 0
    interpolation_time: int = 200
    keyframes: List[Keyframe] = field(default_factory=list)
    # per node: (marker, [8 s16 per key: x y z flag qx qy qz qw])
    tracks: List[Tuple[int, List[Tuple[int, ...]]]] = field(default_factory=list)

    def key_transform(self, node, key):
        v = self.tracks[node][1][key]
        return decode_key(v)


@dataclass
class Socket:
    rotation: Tuple[float, float, float, float]
    position: Vec3
    node: int
    name_hash: int        # with the file's multiplier
    name_hash1: int       # with multiplier 1
    pad: Tuple[int, int] = (0, 0)


@dataclass
class Model:
    header_rest: bytes = b"\0" * 14   # 0x06..0x14 (zeros)
    offsets_offset: int = 0x10
    field_30: int = 0x10
    counts: Optional[List[int]] = None   # as stored (None: computed on write)
    command: str = ""
    radius: float = 0.0
    hash_mult: int = 1
    unknown: int = 0
    lod_distances: List[float] = field(default_factory=list)
    pieces: List[Piece] = field(default_factory=list)
    nodes: List[Node] = field(default_factory=list)
    weight_sets: List[WeightSet] = field(default_factory=list)
    child_models: List[str] = field(default_factory=list)
    animations: List[Animation] = field(default_factory=list)
    sockets: List[Socket] = field(default_factory=list)

    @property
    def lod_count(self):
        return len(self.pieces[0].lods) if self.pieces else len(self.lod_distances) + 1

    def node_parents(self):
        """Parent index per node (-1 for the root), from the depth-first order
        and child counts."""
        parents = [-1] * len(self.nodes)
        stack = []
        for i, n in enumerate(self.nodes):
            while stack and stack[-1][1] == 0:
                stack.pop()
            if stack:
                parents[i] = stack[-1][0]
                stack[-1][1] -= 1
            stack.append([i, n.child_count])
        return parents

    # names
    def piece_name(self, p, names=None):
        n = (names or default_names()).lookup(p.name_hash, self.hash_mult)
        if n is None:
            # rigid pieces are often named after their node
            n = next((x.name for x in self.nodes if name_hash(x.name, self.hash_mult) == p.name_hash), None)
        return n if n is not None else UNNAMED % p.name_hash

    def animation_name(self, a, names=None):
        return (names or default_names()).name(a.name_hash, self.hash_mult)

    def weight_set_name(self, ws, names=None):
        return (names or default_names()).name(ws.name_hash, self.hash_mult)

    def socket_name(self, s, names=None):
        names = names or default_names()
        n = names.lookup(s.name_hash, self.hash_mult)
        if n is None:
            n = names.lookup(s.name_hash1, 1)
        return n if n is not None else UNNAMED % s.name_hash

    def computed_counts(self):
        """The header's 14 counts from what the file holds (see PS2LTB.md).
        Shipped files often count more weights and strings: they were
        counted on the PC model the PS2 file was made from."""
        # as the PC header counts them: the distinct non-empty command string,
        # node, animation and child-model names and keyframe commands (607 of
        # the 613 US-disc models whose names all resolve); an animation name
        # the dictionary doesn't have isn't counted
        strings = set()
        anims = [self.animation_name(a) for a in self.animations]
        for s in [self.command] + [n.name for n in self.nodes] + (self.child_models or []) + \
                [n for n in anims if not n.startswith("#")] + \
                [k.command for a in self.animations for k in a.keyframes]:
            if s:
                strings.add(s)
        tris = verts = weights = 0
        for p in self.pieces:
            for l in p.lods:
                tris += l.triangle_count()
                if l.kind == SKEL_MESH:
                    verts += len(l.uniques)
                    weights += sum(sum(1 for b, w in zip(u.bones, u.weights) if b != 0xFF and w) for u in l.uniques)
                else:
                    # the strip vertices told apart by position and normal
                    # (as PC vertices are; this matches all but one file)
                    k = len({(v.position, v.normal) for s in l.strips() for v in s.vertices})
                    verts += k
                    weights += k
        return [sum(len(a.keyframes) for a in self.animations), len(self.animations), len(self.nodes),
                len(self.pieces), len(self.child_models or []) + 1, tris, verts, weights, self.lod_count,
                len(self.sockets), len(self.weight_sets), len(strings), sum(len(s) for s in strings), 0]


def decode_key(v):
    """8 s16 -> ((x, y, z), (qx, qy, qz, qw)): position / 16 when the flag is
    set, else / 4096; quaternion / 16384."""
    sc = 16.0 if v[3] else 4096.0
    return ((v[0] / sc, v[1] / sc, v[2] / sc),
            (v[4] / 16384.0, v[5] / 16384.0, v[6] / 16384.0, v[7] / 16384.0))


def _s16(x):
    return max(-32768, min(32767, int(round(x))))


def encode_key(position, rotation):
    """((x, y, z), (qx, qy, qz, qw)) -> 8 s16; the fine scale (1/4096) when
    the position fits, else 1/16 with the flag set."""
    fine = all(abs(c * 4096.0) < 32767.5 for c in position)
    sc = 4096.0 if fine else 16.0
    return (_s16(position[0] * sc), _s16(position[1] * sc), _s16(position[2] * sc), 0 if fine else 1,
            _s16(rotation[0] * 16384.0), _s16(rotation[1] * 16384.0), _s16(rotation[2] * 16384.0),
            _s16(rotation[3] * 16384.0))


# --------------------------------------------------------------------- #
#  Reading

def _read_chain(d, pos, lod):
    """The DMA chain from `pos`: buffers of strips. Returns the end."""
    start = pos
    vu_addr = {}   # qword offset in the chain of each strip vertex's position
    while True:
        lo, = struct.unpack_from("<Q", d, pos)
        qwc, tid = lo & 0xFFFF, (lo >> 28) & 7
        p, end = pos + 8, pos + 16 + qwc * 16
        mem = []
        while p < end:
            w, = struct.unpack_from("<I", d, p)
            cmd, num = (w >> 24) & 0x7F, (w >> 16) & 0xFF
            p += 4
            if cmd & 0x60 == 0x60:
                if cmd != VIF_UNPACK_V4_32:
                    raise PS2LTBError("VIF UNPACK 0x%02x not handled" % cmd)
                n = num or 256
                mem += [p + 16 * i for i in range(n)]
                p += 16 * n
            elif cmd not in (VIF_NOP, VIF_FLUSH, VIF_MSCALF):
                raise PS2LTBError("VIF code 0x%02x not handled" % cmd)
        buf = Buffer()
        nm1, nverts = struct.unpack_from("<2I", d, mem[0])
        buf.header_rest = bytes(d[mem[0] + 8:mem[0] + 16])
        a = 1
        got = 0
        while True:
            glo, ghi = struct.unpack_from("<QQ", d, mem[a])
            nloop, eop = glo & 0x7FFF, (glo >> 15) & 1
            st = Strip(reversed=bool(ghi & GIF_REVERSED), gif_lo_rest=glo & ~0xFFFF, gif_hi=ghi)
            a += 1
            for _ in range(nloop):
                o = mem[a]
                f = struct.unpack_from("<11fI", d, o)
                st.vertices.append(StripVertex(f[0:4], f[4:8], f[8:11], f[11]))
                vu_addr[(o - start) // 16] = st.vertices[-1]
                a += 3
            buf.strips.append(st)
            got += nloop
            if eop:
                break
        if got != nverts or len(buf.strips) != nm1 + 1:
            raise PS2LTBError("VU buffer: %d strips, %d vertices; header says %d, %d"
                              % (len(buf.strips), got, nm1 + 1, nverts))
        lod.buffers.append(buf)
        pos = end
        if tid in (0, 6, 7):
            return pos, vu_addr


def read(data) -> Model:
    d = bytes(data)
    r = Reader(d)
    ftype, ver = r.unpack("IH")
    if ftype != FILE_TYPE or ver != VERSION:
        raise PS2LTBError("not a PS2 model (file type %d, version %d; want 2, 16)" % (ftype, ver))
    m = Model()
    m.header_rest = r.raw(14)
    m.offsets_offset, off_pieces, off_nodes, off_children, off_anims, off_sockets, size = r.unpack("7I")
    m.field_30, = r.unpack("I")
    m.counts = list(r.unpack("14I"))
    if size != len(d):
        raise PS2LTBError("file size field %d, file is %d bytes" % (size, len(d)))
    m.command = r.string()
    m.radius, m.hash_mult, m.unknown, ndist = r.unpack("fIII")
    m.lod_distances = list(r.unpack("%df" % ndist))
    nlods = m.counts[8]

    def at(where, what):
        if r.pos != where:
            raise PS2LTBError("%s: at 0x%x, the header says 0x%x" % (what, r.pos, where))

    at(off_pieces, "pieces")
    for _ in range(r.u32()):
        p = Piece()
        p.name_hash, = r.unpack("I")
        p.colors = r.unpack("12f")
        p.texture, = r.unpack("I")
        p.unknown = r.unpack("3I")
        for _ in range(nlods):
            lod = LOD()
            lod.kind = r.u32()
            if lod.kind == SKEL_MESH:
                lod.stored_qwords, lod.stored_wmap_entries, nuniq, nbones = r.unpack("4I")
            elif lod.kind == RIGID_MESH:
                lod.stored_qwords, lod.node = r.unpack("2I")
            else:
                raise PS2LTBError("mesh type %d not handled" % lod.kind)
            chain_start = r.pos
            r.pos, vu_addr = _read_chain(d, r.pos, lod)
            if (r.pos - chain_start) // 16 != lod.stored_qwords:
                raise PS2LTBError("chain of %d qwords, the LOD says %d"
                                  % ((r.pos - chain_start) // 16, lod.stored_qwords))
            if lod.kind == SKEL_MESH:
                vals = r.unpack("%dH" % lod.stored_wmap_entries)
                nb = 2 * lod.stored_wmap_entries
                lod.wmap_pad = r.raw(((nb + 15) & ~15) - nb)
                i = ui = 0
                while i < len(vals):
                    k = vals[i]
                    for q in vals[i + 1:i + 1 + k]:
                        v = vu_addr.get(q)
                        if v is None:
                            raise PS2LTBError("weight map: no strip vertex at qword %d" % q)
                        v.unique = ui
                    i += 1 + k
                    ui += 1
                if ui != nuniq:
                    raise PS2LTBError("weight map of %d vertices, the LOD says %d" % (ui, nuniq))
                for _ in range(nuniq):
                    f = r.unpack("8f")
                    lod.uniques.append(UniqueVertex(f[0:4], f[4:8]))
                lod.bone_map = list(r.unpack("%dI" % nbones))
                for u in lod.uniques:
                    w = r.unpack("4h4B")
                    u.weights, u.bones = w[0:4], w[4:8]
            p.lods.append(lod)
        m.pieces.append(p)

    at(off_nodes, "nodes")
    for _ in range(m.counts[2]):
        name = r.string()
        mat = r.unpack("16f")
        h, nch, idx, flags = r.unpack("IIHH")
        m.nodes.append(Node(name, mat, h, nch, idx, flags))
    for _ in range(r.u32()):
        h, k = r.unpack("2I")
        m.weight_sets.append(WeightSet(h, list(r.unpack("%df" % k))))

    at(off_children, "child models")
    n = r.u32()
    m.child_models = [r.string() for _ in range(max(n - 1, 0))]
    if n == 0:
        m.child_models = None   # (not seen; kept apart from 1 so it writes back)

    at(off_anims, "animations")
    for _ in range(r.u32()):
        a = Animation()
        a.dims = r.vec3()
        a.translation = r.vec3()
        a.name_hash, a.interpolation_time, nk = r.unpack("3I")
        for _ in range(nk):
            t = r.u32()
            a.keyframes.append(Keyframe(t, r.string()))
        for _ in range(len(m.nodes)):
            marker = r.u32()
            a.tracks.append((marker, [r.unpack("8h") for _ in range(nk)]))
        m.animations.append(a)

    at(off_sockets, "sockets")
    for _ in range(r.u32()):
        f = r.unpack("7f")
        pad, node, h, h1, pad2 = r.unpack("5I")
        m.sockets.append(Socket(f[0:4], f[4:7], node, h, h1, (pad, pad2)))
    if r.pos != len(d):
        raise PS2LTBError("%d bytes after the sockets" % (len(d) - r.pos))
    return m


def read_file(path) -> Model:
    with open(path, "rb") as fh:
        return read(fh.read())


def is_ps2_model(data) -> bool:
    return len(data) >= 6 and struct.unpack_from("<IH", data, 0) == (FILE_TYPE, VERSION)


# --------------------------------------------------------------------- #
#  Writing

def _write_chain(w, lod):
    """The LOD's buffers as a DMA chain. Returns {id(strip vertex): qword
    offset of its position in the chain}, for the weight map."""
    start = w.tell()
    where = {}
    for bi, buf in enumerate(lod.buffers):
        last = bi == len(lod.buffers) - 1
        # items: the header qword, then per strip a GIF tag and 3 qwords a vertex
        items = []
        nverts = sum(len(s.vertices) for s in buf.strips)
        items.append((struct.pack("<2I", len(buf.strips) - 1, nverts) + buf.header_rest, None))
        for si, s in enumerate(buf.strips):
            lo = (len(s.vertices) & 0x7FFF) | ((1 if si == len(buf.strips) - 1 else 0) << 15) | s.gif_lo_rest
            items.append((struct.pack("<QQ", lo, s.gif_hi), None))
            for v in s.vertices:
                items.append((struct.pack("<11fI", *v.position, *v.normal, *v.stq, v.extra), v))
        chunks, cur, n = [], [], 0
        for it in items:
            size = len(it[0]) // 16
            if n + size > MAX_UNPACK:
                chunks.append(cur)
                cur, n = [], 0
            cur.append(it)
            n += size
        chunks.append(cur)
        qwc = sum(1 + sum(len(it[0]) // 16 for it in c) for c in chunks) + 1
        tag = w.tell()
        # the tag: qwc, id, and its upper half's two VIF codes
        w.pack("QII", qwc | ((DMA_RET if last else DMA_CNT) << 28),
               (VIF_FLUSH << 24) if bi else (VIF_NOP << 24), VIF_NOP << 24)
        addr = VU_BASE
        for c in chunks:
            nq = sum(len(it[0]) // 16 for it in c)
            # NOPs, so the UNPACK's data starts on a qword
            w.pack("4I", VIF_NOP, VIF_NOP, VIF_NOP, (VIF_UNPACK_V4_32 << 24) | ((nq & 0xFF) << 16) | addr)
            for data, v in c:
                if v is not None:
                    where[id(v)] = (w.tell() - start) // 16
                w.raw(data)
            addr += nq
        w.pack("4I", VIF_NOP, VIF_NOP, VIF_NOP, VIF_MSCALF << 24)
        assert w.tell() - tag == 16 * (qwc + 1), (w.tell() - tag, qwc)
    return where, (w.tell() - start) // 16


def write(m: Model) -> bytes:
    w = Writer()
    w.pack("IH", FILE_TYPE, VERSION)
    w.raw(m.header_rest)
    hdr_offsets = w.tell()
    w.pack("7I", m.offsets_offset, 0, 0, 0, 0, 0, 0)
    w.u32(m.field_30)
    counts_at = w.tell()
    w.pack("14I", *([0] * 14))
    w.string(m.command)
    w.pack("fIII", m.radius, m.hash_mult, m.unknown, len(m.lod_distances))
    w.pack("%df" % len(m.lod_distances), *m.lod_distances)

    offs = []
    offs.append(w.tell())
    w.u32(len(m.pieces))
    for p in m.pieces:
        w.u32(p.name_hash)
        w.pack("12f", *p.colors)
        w.u32(p.texture)
        w.pack("3I", *p.unknown)
        for lod in p.lods:
            w.u32(lod.kind)
            head = w.tell()
            if lod.kind == SKEL_MESH:
                w.pack("4I", 0, 0, len(lod.uniques), len(lod.bone_map))
            else:
                w.pack("2I", 0, lod.node)
            where, qwords = _write_chain(w, lod)
            if lod.kind == SKEL_MESH:
                groups = [[] for _ in lod.uniques]
                for s in lod.strips():
                    for v in s.vertices:
                        if not 0 <= v.unique < len(groups):
                            raise PS2LTBError("a strip vertex copies no unique vertex")
                        groups[v.unique].append(where[id(v)])
                vals = []
                for g in groups:
                    vals.append(len(g))
                    vals += g
                if max(vals, default=0) > 0xFFFF:
                    raise PS2LTBError("chain too long for the weight map's u16 offsets")
                w.pack("%dH" % len(vals), *vals)
                nb = 2 * len(vals)
                pad = ((nb + 15) & ~15) - nb
                w.raw(lod.wmap_pad if len(lod.wmap_pad) == pad else b"\0" * pad)
                for u in lod.uniques:
                    w.pack("8f", *u.position, *u.normal)
                w.pack("%dI" % len(lod.bone_map), *lod.bone_map)
                for u in lod.uniques:
                    w.pack("4h4B", *u.weights, *u.bones)
                struct.pack_into("<2I", w.buf, head, qwords, len(vals))
            else:
                struct.pack_into("<I", w.buf, head, qwords)

    offs.append(w.tell())
    for n in m.nodes:
        w.string(n.name)
        w.pack("16f", *n.bind_matrix)
        w.pack("IIHH", n.name_hash, n.child_count, n.index, n.flags)
    w.u32(len(m.weight_sets))
    for ws in m.weight_sets:
        w.pack("2I", ws.name_hash, len(ws.weights))
        w.pack("%df" % len(ws.weights), *ws.weights)

    offs.append(w.tell())
    if m.child_models is None:
        w.u32(0)
    else:
        w.u32(len(m.child_models) + 1)
        for c in m.child_models:
            w.string(c)

    offs.append(w.tell())
    w.u32(len(m.animations))
    for a in m.animations:
        w.vec3(a.dims)
        w.vec3(a.translation)
        w.pack("3I", a.name_hash, a.interpolation_time, len(a.keyframes))
        for k in a.keyframes:
            w.u32(k.time)
            w.string(k.command)
        if len(a.tracks) != len(m.nodes):
            raise PS2LTBError("animation with %d tracks for %d nodes" % (len(a.tracks), len(m.nodes)))
        for marker, keys in a.tracks:
            w.u32(marker)
            if len(keys) != len(a.keyframes):
                raise PS2LTBError("a track with %d keys for %d keyframes" % (len(keys), len(a.keyframes)))
            for v in keys:
                w.pack("8h", *v)

    offs.append(w.tell())
    w.u32(len(m.sockets))
    for s in m.sockets:
        w.pack("7f", *s.rotation, *s.position)
        w.pack("5I", s.pad[0], s.node, s.name_hash, s.name_hash1, s.pad[1])

    struct.pack_into("<6I", w.buf, hdr_offsets + 4, *offs, w.tell())
    counts = m.counts if m.counts is not None else m.computed_counts()
    struct.pack_into("<14I", w.buf, counts_at, *counts)
    return w.getvalue()


def write_file(m: Model, path):
    with open(path, "wb") as fh:
        fh.write(write(m))


# --------------------------------------------------------------------- #
#  Building geometry

def stripify(triangles):
    """Triangle strips from triangles ((a, b, c) vertex indices, PC clockwise
    order): a greedy walk, each strip as long as it goes. Returns
    [(reversed, [vertex indices])], whose Strip.triangles() give back every
    triangle with its winding."""
    tris = [tuple(t) for t in triangles]
    by_edge = {}
    for ti, (a, b, c) in enumerate(tris):
        for e in ((a, b), (b, c), (c, a)):
            by_edge.setdefault(e, []).append(ti)
    used = [False] * len(tris)
    out = []
    for start in range(len(tris)):
        if used[start]:
            continue
        used[start] = True
        a, b, c = tris[start]
        seq = [a, b, c]       # triangle 0 is (0, 1, 2) unswapped: not reversed
        while True:
            k = len(seq)      # the next triangle is (k-2, k-1, k)
            u, v = seq[-2], seq[-1]
            # its winding: swapped when k is odd -> (v, u, x) else (u, v, x)
            want = (v, u) if k % 2 == 1 else (u, v)
            nxt = None
            for ti in by_edge.get(want, ()):
                if not used[ti]:
                    t = tris[ti]
                    i = t.index(want[0])
                    if t[(i + 1) % 3] == want[1]:
                        nxt = (ti, t[(i + 2) % 3])
                        break
            if nxt is None:
                break
            used[nxt[0]] = True
            seq.append(nxt[1])
        out.append((False, seq))
    return out


def buffers_from_strips(strips, max_qwords=MAX_BUFFER):
    """Strips (Strip objects) packed into VU buffers greedily, whole strips,
    at most max_qwords each (header and GIF tags included)."""
    bufs, cur, n = [], [], 1
    for s in strips:
        size = 1 + 3 * len(s.vertices)
        if size + 1 > max_qwords:
            raise PS2LTBError("a strip of %d vertices doesn't fit a VU buffer" % len(s.vertices))
        if cur and n + size > max_qwords:
            bufs.append(Buffer(cur))
            cur, n = [], 1
        cur.append(s)
        n += size
    if cur:
        bufs.append(Buffer(cur))
    return bufs
