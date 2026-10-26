"""DHNP's wrapped ABC and D3D LTBs, converted to the shared ABC model.

Format reference: StenApp/io_scene_lithtech, commit 0969529,
research/LTB_D3D_MODEL_FILE_DHNP.bt and src/reader_ltb_dhnp.py.
This reader keeps file coordinates and converts D3D weights to node-local
points. Export uses the existing ABC/PC LTB writers, not the DHNP wrapper.
"""

from . import abc
from .binio import Reader

PREFIX = b"\x09\x00LTBHeader"


def is_model(data):
    return bytes(data[:len(PREFIX)]) == PREFIX


def _inverse(values):
    rows = [list(values[i * 4:i * 4 + 4]) + [float(i == j) for j in range(4)] for i in range(4)]
    for i in range(4):
        pivot = max(range(i, 4), key=lambda j: abs(rows[j][i]))
        if abs(rows[pivot][i]) < 1e-12:
            raise abc.ABCError("DHNP node has a singular bind matrix")
        rows[i], rows[pivot] = rows[pivot], rows[i]
        divisor = rows[i][i]
        rows[i] = [v / divisor for v in rows[i]]
        for j in range(4):
            if i != j:
                factor = rows[j][i]
                rows[j] = [a - factor * b for a, b in zip(rows[j], rows[i])]
    return [row[4:] for row in rows]


def _d3d_lod(r):
    kind, size = r.unpack("2I")
    end = r.pos + size
    if size < 112 or end > len(r.data):
        raise abc.ABCError("DHNP LOD has an invalid size")
    payload = Reader(r.data[r.pos:end])
    r.pos = end
    payload.raw(68)  # colour values and tool padding
    texture = payload.u32()
    payload.raw(12)
    power, specular = payload.unpack("2f")
    payload.u32()
    nv, nt, per_triangle, per_vertex = payload.unpack("4I")
    if kind not in (4, 5):
        raise abc.ABCError("unsupported DHNP mesh type %d" % kind)
    lod = abc.LOD()
    lod.texture_index = texture
    vertex_format = payload.u8() if kind == 5 else None
    if vertex_format not in (None, 0, 1):
        raise abc.ABCError("unsupported DHNP vertex format %d" % vertex_format)
    if vertex_format == 1:
        if per_vertex not in (2, 3, 4):
            raise abc.ABCError("unsupported DHNP blend count %d" % per_vertex)
        payload.raw(8)  # stream descriptors
    bone = payload.u32() if kind == 4 else 0
    # Locations are model-space here; fill each weight's local point once
    # the node chunk has been read.
    for _ in range(nv):
        pos = payload.vec3()
        if vertex_format == 1:
            biases = list(payload.unpack("%df" % (per_vertex - 1)))
            biases.append(1.0 - sum(biases))
            indices = payload.unpack("4B")
            weights = [abc.Weight(i, (0, 0, 0), w) for i, w in zip(indices, biases)]
        else:
            if vertex_format == 0:
                if per_triangle != 1:
                    payload.f32()
                if per_triangle not in (1, 2):
                    payload.u32()
            weights = [abc.Weight(bone, (0, 0, 0), 1.0)]
        lod.vertices.append(abc.Vertex(weights, 0, pos, payload.vec3()))
    uvs = [payload.unpack("2f") for _ in range(nv)]
    for _ in range(nt):
        indices = payload.unpack("3H")
        if any(i >= nv for i in indices):
            raise abc.ABCError("DHNP triangle references a missing vertex")
        lod.triangles.append(abc.Triangle(indices, tuple(uvs[i] for i in indices)))
    remaining = len(payload.data) - payload.pos
    if vertex_format == 0 and remaining >= 4 and (remaining - 4) % 12 == 0:
        count = payload.u32()
        if count * 12 != remaining - 4:
            raise abc.ABCError("DHNP bone-set count doesn't match its LOD size")
        for _ in range(count):
            first, length = payload.unpack("2H")
            bones = payload.unpack("4B")
            payload.u32()
            bone = next((i for i in bones if i != 255), 0)
            if first + length > nv:
                raise abc.ABCError("DHNP bone set references missing vertices")
            for v in lod.vertices[first:first + length]:
                v.weights[0].node = bone
    if payload.pos != len(payload.data):
        raise abc.ABCError("DHNP LOD has %d unconsumed bytes" % (len(payload.data) - payload.pos))
    return lod, power, specular


def read(data):
    r = Reader(data)
    if r.string() != "LTBHeader":
        raise abc.ABCError("not a DHNP model")
    r.u32()
    kind, wrapper_version = r.unpack("2H")
    r.raw(16)
    if kind not in (1, 4) or wrapper_version != 3:
        raise abc.ABCError("unsupported DHNP wrapper type/version %d/%d" % (kind, wrapper_version))
    chunks = abc.read_chunks(data, start=r.pos)
    if tuple(name for name, _, _ in chunks) != abc.CHUNK_ORDER:
        raise abc.ABCError("unexpected DHNP chunk order")
    sections = {name: (start, end) for name, start, end in chunks}

    def section(name):
        start, end = sections[name]
        return Reader(data[start:end])

    def done(name, reader):
        if reader.pos != len(reader.data):
            raise abc.ABCError("DHNP %s chunk has %d unconsumed bytes" % (name, len(reader.data) - reader.pos))

    m = abc.Model(version=12)
    r = section("Header")
    version = r.u32()
    if version not in (9, 10, 11, 12, 13):
        raise abc.ABCError("unsupported DHNP model version %d" % version)
    counts = r.unpack("13I")
    if version == 13:
        r.u32()
    m.command_string = r.string()
    m.internal_radius = r.f32()
    distances = r.u32()
    r.raw(60)
    m.lod_distances = list(r.unpack("%df" % distances)) if distances else []
    done("Header", r)
    if counts[8] != distances + 1:
        raise abc.ABCError("DHNP LOD count doesn't match its distance table")
    m.source_format = "DHNP ABC" if kind == 4 else "DHNP D3D"
    m.source_version = version

    r = section("Pieces")
    if kind == 4:
        r.u32()  # total weights
    for _ in range(r.u32()):
        p = abc.Piece(name="")
        if kind == 4:
            p.texture_index = r.u16()
            p.specular_power, p.specular_scale = r.unpack("2f")
            if version >= 10:
                p.lod_weight = r.f32()
            n = r.u16()
            p.reserved = list(r.unpack("%dI" % n)) if n else []
        p.name = r.string()
        for _ in range(counts[8]):
            if kind == 4:
                lod = abc._read_lod(r)
            else:
                lod, power, specular = _d3d_lod(r)
                if not p.lods:
                    p.texture_index, p.specular_power, p.specular_scale = lod.texture_index, power, specular
            p.lods.append(lod)
        m.pieces.append(p)
    done("Pieces", r)

    r = section("Nodes")
    m.root = abc._read_node(r)
    nodes = m.nodes
    if len(nodes) != counts[2]:
        raise abc.ABCError("DHNP node count doesn't match its header")
    for _ in range(r.u32()):
        name, count = r.string(), r.u32()
        m.weight_sets.append(abc.WeightSet(name, list(r.unpack("%df" % count)) if count else []))
    done("Nodes", r)
    if kind == 1:
        inverses = [_inverse(n.bind_matrix) for n in nodes]
        for p in m.pieces:
            for lod in p.lods:
                for v in lod.vertices:
                    point = (*v.location, 1.0)
                    for w in v.weights:
                        if w.node >= len(nodes):
                            raise abc.ABCError("DHNP weight references a missing node")
                        w.location = tuple(sum(row[j] * point[j] for j in range(4)) * w.bias
                                           for row in inverses[w.node][:3])

    r = section("ChildModels")
    for _ in range(r.u16()):
        name, save_index = r.string(), r.u32()
        relation = [(r.vec3(), r.quat()) for _ in nodes]
        m.child_models.append(abc.ChildModel(name, save_index, relation))
    done("ChildModels", r)

    r = section("Animation")
    for _ in range(r.u32()):
        a = abc.Animation(name="", dims=r.vec3())
        a.name = r.string()
        keys = r.i32()
        if keys == -1:
            a.interpolation_time = r.u32() if version >= 12 else 200
            keys = r.u32()
        if keys < 0:
            raise abc.ABCError("DHNP animation has a negative key count")
        a.keyframes = [abc.Keyframe(r.u32(), r.string()) for _ in range(keys)]
        for _ in nodes:
            if version == 13:
                r.i32()  # node-track marker
            transforms = []
            for _ in range(keys):
                transforms.append((r.vec3(), r.quat()))
                if version == 13:
                    r.raw(8)  # additional track values, not present in ABC 12
            a.node_transforms.append(transforms)
        m.animations.append(a)
    done("Animation", r)

    r = section("Sockets")
    for _ in range(r.u32()):
        node, name = r.u32(), r.string()
        m.sockets.append(abc.Socket(node, name, r.quat(), r.vec3()))
    done("Sockets", r)
    r = section("AnimBindings")
    for _ in m.child_models:
        m.anim_bindings.append([abc.AnimBinding(r.string(), r.vec3(), r.vec3()) for _ in range(r.u32())])
    done("AnimBindings", r)
    return m


def read_file(path):
    with open(path, "rb") as f:
        return read(f.read())
