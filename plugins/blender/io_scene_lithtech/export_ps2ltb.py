"""Export an armature and its meshes as a PS2 NOLF model (.ltb, file type 2,
version 16).

A mesh imported by import_ps2ltb and left as it was (the same faces, UVs
and vertex count) keeps its strips, its raw positions and normals and its
stored weights. Anything else is rebuilt:

  * triangles (n-gons as fans) into strips by a greedy walk, packed into
    VU buffers of at most 844 qwords (the largest any shipped file has),
    split into UNPACKs of at most 255 qwords on whole vertices: the
    shipped files' packing, which lithtech/ps2ltb.py reproduces for every
    one of them
  * a mesh weighted to one bone only is rigid (vertices in that node's
    space); anything else is skeletal, with up to four weights a vertex
    (the strongest, normalised, in 1/4096) over the nodes it uses
  * strip vertices carry 0 in the u32 after q (what most shipped vertices
    have; its meaning is unknown)

Nothing written by this path has been run on a PS2.
"""

import base64
import json
import struct

import bpy
from mathutils import Matrix, Vector

from .lithtech import abc, ps2ltb
from . import convert
from . import export_abc
from . import import_ps2ltb

ExportError = export_abc.ExportError


def _attr(me, name, kind):
    a = me.attributes.get(name)
    if a is None or a.domain != 'POINT':
        return None
    if kind == 'color' and a.data_type == 'FLOAT_COLOR':
        out = [0.0] * (4 * len(me.vertices))
        a.data.foreach_get("color", out)
        return [tuple(out[4 * i:4 * i + 4]) for i in range(len(me.vertices))]
    if kind == 'int' and a.data_type == 'INT':
        out = [0] * len(me.vertices)
        a.data.foreach_get("value", out)
        return out
    return None


def _inverse(m16):
    m = Matrix((m16[0:4], m16[4:8], m16[8:12], m16[12:16]))
    try:
        m = m.inverted()
    except ValueError:
        m = Matrix.Identity(4)
    return tuple(m[i][j] for i in range(4) for j in range(4))


def _apply(m, v, w=1.0):
    return tuple(m[4 * r] * v[0] + m[4 * r + 1] * v[1] + m[4 * r + 2] * v[2] + m[4 * r + 3] * w for r in range(3))


def _same_key(was, position, rotation):
    """Whether the action's key (floats) is the stored key: its position
    within the stored key's step, and the same rotation within 0.16 degrees
    (keys go through float32 matrices on the way in), whatever the stored
    quaternion's length or sign."""
    p, q = ps2ltb.decode_key(was)
    step = 1.0 / 16.0 if was[3] else 1.0 / 4096.0
    if any(abs(x - y) > step * 0.51 + 1e-6 for x, y in zip(p, position)):
        return False
    lq = sum(x * x for x in q) ** 0.5
    lr = sum(x * x for x in rotation) ** 0.5
    if not lq or not lr:
        return lq == lr
    # near half a turn (w about 0) a matrix gives its quaternion back least
    # precisely: there up to 0.9 degrees
    tol = 3e-5 if abs(q[3]) / lq < 0.01 else 1e-6
    if abs(sum(x * y for x, y in zip(q, rotation))) / (lq * lr) > 1.0 - tol:
        return True
    if abs(lq - 1.0) > 1e-3:
        # far from unit length: the import made a matrix of it as it was,
        # and the action holds that matrix's rotation
        mq = abc.quat_to_matrix((0.0, 0.0, 0.0), q)
        r = Matrix([mq[0:4], mq[4:8], mq[8:12], mq[12:16]]).decompose()[1]
        return abs(r.x * rotation[0] + r.y * rotation[1] + r.z * rotation[2] + r.w * rotation[3]) / lr > 1.0 - tol
    return False


def _close(a, b, tol):
    return all(abs(x - y) <= tol for x, y in zip(a, b))


def _vertex_data(obj, me, to_arm, scale, nodes, node_index):
    """Per vertex: model-space position and normal, [(node, weight)], and
    whether the normal is still the one the import gave it."""
    me.calc_loop_triangles()
    normals = [Vector((0.0, 0.0, 0.0)) for _ in me.vertices]
    for li, loop in enumerate(me.loops):
        normals[loop.vertex_index] += me.corner_normals[li].vector
    imported = me.attributes.get("lt_ps2_import_normal")
    same_normal = [False] * len(me.vertices)
    if imported is not None and imported.domain == 'POINT' and imported.data_type == 'FLOAT_VECTOR':
        was = [0.0] * (3 * len(me.vertices))
        imported.data.foreach_get("vector", was)
        same_normal = [(normals[i] - Vector(was[3 * i:3 * i + 3])).length < 1e-4 for i in range(len(me.vertices))]
    rot = to_arm.to_3x3()
    pos, nrm = [], []
    for v, n in zip(me.vertices, normals):
        pos.append(convert.vec_from_blender(to_arm @ v.co, scale))
        n = (rot @ n)
        nrm.append(convert.normal_from_blender(n.normalized() if n.length else Vector((0.0, 0.0, 1.0))))
    groups = {g.index: node_index.get(g.name) for g in obj.vertex_groups}
    weights = []
    for v in me.vertices:
        ws = [(groups.get(g.group), g.weight) for g in v.groups if groups.get(g.group) is not None and g.weight > 0]
        weights.append(ws)
    return pos, nrm, weights, same_normal


def _quantise_weights(ws):
    """[(node, w)] -> the four strongest as (nodes, s16 weights summing to 4096)."""
    ws = sorted(ws, key=lambda x: -x[1])[:4]
    total = sum(w for _, w in ws) or 1.0
    q = [int(round(w / total * 4096.0)) for _, w in ws]
    if q:
        q[0] += 4096 - sum(q)
    return [n for n, _ in ws], q


def _corner_uvs(me):
    uv = me.uv_layers.active
    out = [(0.0, 0.0)] * len(me.loops)
    if uv is not None:
        flat = [0.0] * (2 * len(me.loops))
        uv.data.foreach_get("uv", flat)
        out = [(flat[2 * i], 1.0 - flat[2 * i + 1]) for i in range(len(me.loops))]
    return out


def _kept_strips(obj, me, uvs, source):
    """The stored strips, when the mesh still has the faces and UVs they
    make: [[(reversed, gif_hi, [(vertex, s, t, extra)])]] per buffer, the
    vertices as the file numbered them (source: per Blender vertex)."""
    if "lt_ps2_strips" not in obj or source is None:
        return None
    try:
        buffers = json.loads(obj["lt_ps2_strips"])
    except ValueError:
        return None
    degenerate = {int(d[0]) for d in json.loads(obj.get("lt_degenerate_faces", "[]"))}
    tris = []
    for b in buffers:
        for rev, gif_hi, verts in b:
            st = ps2ltb.Strip(reversed=bool(rev))
            st.vertices = [None] * len(verts)
            for a, b_, c in st.triangles():
                tris.append((verts[a], verts[c], verts[b_]))   # Blender winding
    keep = [t for i, t in enumerate(tris) if i not in degenerate]
    if len(keep) != len(me.polygons):
        return None
    for poly, t in zip(me.polygons, keep):
        if poly.loop_total != 3 or tuple(source[v] for v in poly.vertices) != tuple(v[0] for v in t):
            return None
        for li, v in zip(poly.loop_indices, t):
            if abs(uvs[li][0] - v[1]) > 1e-5 * max(1.0, abs(v[1])) or \
                    abs(uvs[li][1] - v[2]) > 1e-5 * max(1.0, abs(v[2])):
                return None
    return buffers


def _new_strips(me, uvs):
    """Strips for the mesh's faces: [(vertex, s, t, extra)] lists."""
    keys, corners = {}, []
    tris = []
    for poly in me.polygons:
        lis = list(poly.loop_indices)
        for k in range(1, len(lis) - 1):
            tri = []
            # Blender (a, b, c) is PC (a, c, b)
            for li in (lis[0], lis[k + 1], lis[k]):
                key = (me.loops[li].vertex_index, round(uvs[li][0], 6), round(uvs[li][1], 6))
                ci = keys.get(key)
                if ci is None:
                    ci = keys[key] = len(corners)
                    corners.append((me.loops[li].vertex_index, uvs[li][0], uvs[li][1], 0))
                tri.append(ci)
            if len(set(tri)) == 3:
                tris.append(tuple(tri))
    out = []
    for rev, seq in ps2ltb.stripify(tris):
        out.append((rev, ps2ltb.GIF_REGS | (ps2ltb.GIF_REVERSED if rev else 0), [corners[i] for i in seq]))
    return out


def _lod_from_object(obj, me, to_arm, scale, m, node_index, default_node):
    pos, nrm, weights, same_normal = _vertex_data(obj, me, to_arm, scale, m.nodes, node_index)
    uvs = _corner_uvs(me)
    raw_p = _attr(me, "lt_ps2_position", 'color')
    raw_n = _attr(me, "lt_ps2_normal", 'color')
    used = {n for ws in weights for n, _ in ws}
    kind = int(obj.get("lt_mesh_kind", 0))
    effector = node_index.get(obj.get("lt_bone_effector", ""), None)
    if kind not in (ps2ltb.RIGID_MESH, ps2ltb.SKEL_MESH) or (kind == ps2ltb.RIGID_MESH and used - {effector}):
        if len(used) <= 1 and all(abs(w - 1.0) < 1e-4 for ws in weights for _, w in ws):
            kind, effector = ps2ltb.RIGID_MESH, (next(iter(used)) if used else default_node)
        else:
            kind = ps2ltb.SKEL_MESH
    if kind == ps2ltb.RIGID_MESH and effector is None:
        effector = next(iter(used)) if used else default_node

    lod = ps2ltb.LOD(kind=kind)
    # each vertex's stored (position, normal), raw where it still fits
    vraw = []
    if kind == ps2ltb.RIGID_MESH:
        lod.node = effector
        bind = m.nodes[effector].bind_matrix
        inv = _inverse(bind)
        for i, (p, n) in enumerate(zip(pos, nrm)):
            rp = raw_p[i] if raw_p else None
            rn = raw_n[i] if raw_n else None
            if rp is not None and _close(_apply(bind, rp), p, 1e-3):
                p4 = rp
            else:
                p4 = _apply(inv, p) + (1.0,)
            if rn is not None and (same_normal[i] or _close(_apply(bind, rn, 0.0), n, 1e-2)):
                n4 = rn
            else:
                n4 = _apply(inv, n, 0.0) + (0.0,)
            vraw.append((p4, n4))
    else:
        for i, (p, n) in enumerate(zip(pos, nrm)):
            rp = raw_p[i] if raw_p else None
            rn = raw_n[i] if raw_n else None
            p4 = rp if rp is not None and _close(rp, p, 1e-3) else tuple(p) + (1.0,)
            n4 = rn if rn is not None and (same_normal[i] or _close(rn, n, 1e-2)) else tuple(n) + (0.0,)
            vraw.append((p4, n4))

    source = _attr(me, "lt_ps2_unique", 'int')
    if source is not None and (not source or min(source) < 0):
        source = None
    buffers = _kept_strips(obj, me, uvs, source)
    if buffers is not None:
        # back to the file's vertices: the first Blender copy of each
        first = {}
        for bi, fi in enumerate(source):
            first.setdefault(fi, bi)
        n = max(source) + 1
        if len(first) != n:
            buffers = None
    if buffers is not None:
        rep = [first[i] for i in range(n)]
        vraw = [vraw[b] for b in rep]
        weights = [weights[b] for b in rep]
        sel = rep
    else:
        sel = list(range(len(me.vertices)))
    if buffers is not None:
        for b in buffers:
            buf = ps2ltb.Buffer()
            for rev, gif_hi, verts in b:
                st = ps2ltb.Strip(reversed=bool(rev), gif_hi=gif_hi)
                for vi, s, t, extra in verts:
                    st.vertices.append(ps2ltb.StripVertex(vraw[vi][0], vraw[vi][1], (s, t, 1.0), extra, vi))
                buf.strips.append(st)
            lod.buffers.append(buf)
    else:
        strips = []
        for rev, gif_hi, verts in _new_strips(me, uvs):
            st = ps2ltb.Strip(reversed=bool(rev), gif_hi=gif_hi)
            for vi, s, t, extra in verts:
                st.vertices.append(ps2ltb.StripVertex(vraw[vi][0], vraw[vi][1], (s, t, 1.0), extra, vi))
            strips.append(st)
        lod.buffers = ps2ltb.buffers_from_strips(strips)

    if kind == ps2ltb.SKEL_MESH:
        raw_w = _attr(me, "lt_ps2_weights", 'color')
        raw_b = _attr(me, "lt_ps2_bones", 'int')
        if raw_w and raw_b:
            raw_w, raw_b = [raw_w[b] for b in sel], [raw_b[b] for b in sel]
        stored_map = [int(x) for x in obj.get("lt_ps2_bone_map", [])]
        kept = None
        if raw_w and raw_b and stored_map:
            kept = []
            for i, ws in enumerate(weights):
                b4 = tuple((raw_b[i] & 0xFFFFFFFF).to_bytes(4, "little"))
                w4 = tuple(int(round(x)) for x in raw_w[i])
                u = ps2ltb.UniqueVertex(vraw[i][0], vraw[i][1], w4, b4)
                try:
                    stored = import_ps2ltb.group_weights(u.node_weights(stored_map))
                except IndexError:
                    kept = None
                    break
                now = dict(ws)
                if set(stored) != set(now) or any(abs(stored[n] - now[n]) > 2e-3 for n in stored):
                    kept = None
                    break
                kept.append(u)
        if kept is not None:
            lod.uniques, lod.bone_map = kept, stored_map
        else:
            bone_map, local = [], {}
            for i, ws in enumerate(weights):
                ns, q = _quantise_weights(ws or [(default_node, 1.0)])
                bones = []
                for n in ns:
                    if n not in local:
                        local[n] = len(bone_map)
                        bone_map.append(n)
                    bones.append(local[n] * 4)
                lod.uniques.append(ps2ltb.UniqueVertex(vraw[i][0], vraw[i][1], tuple(q + [0] * (4 - len(q))),
                                                       tuple(bones + [0xFF] * (4 - len(bones)))))
            if len(bone_map) * 4 > 0xFF:
                raise ExportError("%s: a skeletal mesh can use at most 63 nodes" % obj.name)
            lod.bone_map = bone_map
    return lod


def save(context, filepath, *, armature=None, scale=1.0, use_selection=False, export_animations=True):
    arm_obj = armature or export_abc.find_armature(context)
    if arm_obj is None:
        raise ExportError("select an armature, or a mesh deformed by one")
    if context.object and context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    fps = context.scene.render.fps / context.scene.render.fps_base
    ours = arm_obj.get("lt_format") == "ps2ltb"

    abc_nodes, node_bones, abc_root = export_abc._build_skeleton(arm_obj, scale)
    m = ps2ltb.Model()
    m.hash_mult = int(arm_obj.get("lt_hash_mult", 9)) if ours else 9
    mult = m.hash_mult
    if ours and "lt_ps2_header_rest" in arm_obj:
        m.header_rest = bytes.fromhex(arm_obj["lt_ps2_header_rest"])
    m.command = arm_obj.get("lt_command_string", "")
    m.radius = float(arm_obj.get("lt_internal_radius", 96.0))
    m.unknown = int(arm_obj.get("lt_ps2_unknown", 0)) if ours else 0
    for n, b in zip(abc_nodes, node_bones):
        bind = n.bind_matrix
        if b is not None and "lt_bind" in b:
            kept = tuple(float(x) for x in b["lt_bind"])
            if max(abs(x - y) for x, y in zip(kept, bind)) < 1e-4:
                bind = kept
        m.nodes.append(ps2ltb.Node(n.name, tuple(bind), ps2ltb.name_hash(n.name, mult), len(n.children),
                                   len(m.nodes), n.flags))
    node_index = {b.name: i for i, b in enumerate(node_bones) if b is not None}

    candidates = context.selected_objects if use_selection else arm_obj.children_recursive
    if not use_selection:
        candidates = set(candidates) | {o for o in context.scene.objects
                                        if any(md.type == 'ARMATURE' and md.object == arm_obj
                                               for md in getattr(o, "modifiers", ()))}
    meshes = export_abc._mesh_objects(arm_obj, candidates)
    depsgraph = context.evaluated_depsgraph_get()
    arm_inv = arm_obj.matrix_world.inverted()
    base = sorted((o for o in meshes if int(o.get("lt_lod", 0)) == 0),
                  key=lambda o: (o.get("lt_piece_index", 1 << 30), o.name))

    def piece_key(o):
        return ("i", o["lt_piece_index"]) if "lt_piece_index" in o else ("n", o.get("lt_piece", o.name))

    extra = {(piece_key(o), int(o.get("lt_lod", 0))): o for o in meshes if int(o.get("lt_lod", 0)) > 0}
    nlods = max([int(o.get("lt_lod_count", 1)) for o in base] + [k[1] + 1 for k in extra] + [1])
    for o in base:
        default_node = node_index.get(o.parent_bone, 0) if o.parent_type == 'BONE' else 0
        p = ps2ltb.Piece(ps2ltb.hash_of(o.get("lt_piece", o.name), mult))
        if "lt_ps2_colors" in o:
            p.colors = tuple(float(x) for x in o["lt_ps2_colors"])
        if "lt_ps2_piece_unknown" in o:
            p.unknown = tuple(int(x) for x in o["lt_ps2_piece_unknown"])
        mat = o.active_material
        p.texture = int(o.get("lt_texture_index", mat.get("lt_texture_index", 0) if mat is not None else 0))
        prev = None
        for li in range(nlods):
            lo = o if li == 0 else extra.get((piece_key(o), li))
            if lo is None:
                # every piece has every LOD: repeat the last one
                p.lods.append(prev)
                continue
            me = export_abc._evaluated_mesh(lo, depsgraph)
            try:
                to_arm = export_abc._to_armature(lo, arm_obj, arm_inv)
                prev = _lod_from_object(lo, me, to_arm, scale, m, node_index, default_node)
            finally:
                bpy.data.meshes.remove(me)
            p.lods.append(prev)
        m.pieces.append(p)
    stored_d = [float(x) for x in arm_obj.get("lt_lod_distances", [])]
    m.lod_distances = (stored_d + [0.0] * nlods)[:nlods - 1]

    # sockets
    for e in sorted(arm_obj.children, key=lambda o: (o.get("lt_index", 1 << 30), o.name)):
        if e.type != 'EMPTY' or e.parent_type != 'BONE' or e.parent_bone not in node_index:
            continue
        if use_selection and not e.select_get():
            continue
        bone = arm_obj.data.bones[e.parent_bone]
        local_b = Matrix.Translation((0.0, bone.length, 0.0)) @ e.matrix_parent_inverse @ e.matrix_basis
        loc, rot, _ = local_b.decompose()
        pos, q = convert.transform_from_blender(Matrix.Translation(loc) @ rot.to_matrix().to_4x4(), scale)
        if "lt_socket_rotation" in e:
            was = e.get("lt_socket_basis_rotation")
            now = e.matrix_basis.to_quaternion()
            if was is not None and abs(abs(sum(x * y for x, y in zip(was, now))) - 1.0) < 1e-6:
                q = tuple(float(c) for c in e["lt_socket_rotation"])
        if "lt_ps2_socket_raw" in e and "lt_ps2_socket_basis" in e:
            was = [float(x) for x in e["lt_ps2_socket_basis"]]
            now = [c for row in e.matrix_basis for c in row]
            if max(abs(x - y) for x, y in zip(was, now)) < 1e-6:
                raw = [float(x) for x in e["lt_ps2_socket_raw"]]
                q, pos = tuple(raw[0:4]), tuple(raw[4:7])
        name = e.get("lt_name", e.name)
        if e.get("lt_ps2_socket_name") == name and "lt_ps2_socket_hashes" in e:
            h, h1 = (int(x, 16) for x in str(e["lt_ps2_socket_hashes"]).split())
        else:
            h, h1 = ps2ltb.hash_of(name, mult), ps2ltb.hash_of(name, 1)
        m.sockets.append(ps2ltb.Socket(tuple(q), tuple(pos), node_index[e.parent_bone], h, h1))

    for ws in json.loads(arm_obj.get("lt_weight_sets_json", "[]")):
        w = ws.get("weights", {})
        m.weight_sets.append(ps2ltb.WeightSet(ps2ltb.hash_of(ws["name"], mult),
                                              [float(w.get(n.name, 0.0)) for n in m.nodes]))
    stored_children = json.loads(arm_obj.get("lt_child_models_json", "[]")) if ours else []
    m.child_models = stored_children

    rest_locals = []
    for b in node_bones:
        if b is None:
            rest_locals.append(Matrix.Identity(4))
        elif b.parent is None:
            rest_locals.append(b.matrix_local.copy())
        else:
            rest_locals.append(b.parent.matrix_local.inverted() @ b.matrix_local)
    if export_animations:
        for a in export_abc._actions_for(arm_obj, node_bones):
            anim, tr = export_abc._animation_from_action(a, arm_obj, abc_nodes, node_bones, rest_locals, scale, fps)
            coarse = a.get("lt_ps2_coarse", "")
            stored = []
            if "lt_ps2_tracks" in a:
                raw = base64.b64decode(a["lt_ps2_tracks"])
                nk = len(anim.keyframes)
                if len(raw) == 16 * nk * len(m.nodes):
                    flat = list(struct.iter_unpack("<8h", raw))
                    stored = [flat[ni * nk:(ni + 1) * nk] for ni in range(len(m.nodes))]
            pa = ps2ltb.Animation(tuple(anim.dims), tuple(a.get("lt_translation", tr)),
                                  ps2ltb.hash_of(a.get("lt_name", a.name), mult), anim.interpolation_time,
                                  [ps2ltb.Keyframe(k.time, k.command) for k in anim.keyframes])
            for ni, per in enumerate(anim.node_transforms):
                fits = all(abs(c * 4096.0) < 32767.5 for p_, _ in per for c in p_)
                use_coarse = (coarse[ni:ni + 1] == "1") if len(coarse) == len(m.nodes) else not fits
                if not fits:
                    use_coarse = True
                keys = []
                for ki, (p_, q_) in enumerate(per):
                    v = ps2ltb.encode_key(p_, q_)
                    if use_coarse and not v[3]:
                        v = tuple(ps2ltb._s16(c * 16.0) for c in p_) + (1,) + v[4:]
                    if stored:
                        was = stored[ni][ki]
                        if _same_key(was, p_, q_):
                            v = was
                    keys.append(v)
                pa.tracks.append((0xFFFFFFFF, keys))
            m.animations.append(pa)
    if not m.animations:
        b = export_abc._bind_animation(abc_nodes, "base", (24.0, 53.0, 24.0))
        pa = ps2ltb.Animation(tuple(b.dims), (0.0, 0.0, 0.0), ps2ltb.name_hash("base", mult), 200,
                              [ps2ltb.Keyframe(0, "")])
        pa.tracks = [(0xFFFFFFFF, [ps2ltb.encode_key(*b.node_transforms[ni][0])]) for ni in range(len(m.nodes))]
        m.animations.append(pa)

    computed = m.computed_counts()
    stored = [int(x) for x in arm_obj.get("lt_ps2_counts", [])] if ours else []
    was = [int(x) for x in arm_obj.get("lt_ps2_counts_computed", [])] if ours else []
    # the stored counts (counted on the PC model the file was made from) while
    # the model computes as it did when imported
    m.counts = stored if stored and was == computed else computed

    ps2ltb.write_file(m, filepath)
    return m, [filepath]
