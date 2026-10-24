"""Export an armature and its meshes as a LithTech .ltb model (NOLF2,
Contract J.A.C.K., Tron 2.0: LTB version 9, model file version 23 unless
the model was imported at another).

The inverse of import_ltb. Pieces, LODs, sockets, weight sets, child
models and actions are gathered as export_abc does. Each LOD becomes a D3D
render object:

  * a mesh imported from an .ltb whose vertex and face counts haven't
    changed keeps its layout: vertex i is D3D vertex i, its bone sets and
    (where the weights are unchanged) its stored blend weights;
  * any other mesh is laid out afresh: one D3D vertex per distinct
    (vertex, UV, normal); a mesh deformed by one bone (or none) is a rigid
    mesh following it, others skeletal, with triangles packed into bone
    sets of up to four bones (the strongest four per triangle are kept).

Animations are written with their lt_compression (new ones: 16-bit
rotations, ANCMPRS_REL_16_ROT_ONLY, as most shipped animations are):
a channel keeps no keys if it is identity throughout, one if it never
changes, else one per keyframe.
"""

import json

import bpy
from mathutils import Matrix, Vector

from .lithtech import ltb
from . import convert
from . import export_abc
from . import import_ltb
from .export_abc import ExportError


IDENT_POS = (0.0, 0.0, 0.0)
IDENT_ROT = (0.0, 0.0, 0.0, 1.0)


def _to_ltb_node(n):
    out = ltb.Node(n.name, n.index, n.flags, tuple(n.bind_matrix))
    out.children = [_to_ltb_node(c) for c in n.children]
    return out


# --------------------------------------------------------------------- #
#  Meshes
# --------------------------------------------------------------------- #

def _point_attr(me, name, kind, width):
    a = me.attributes.get(name)
    if a is None or a.domain != 'POINT' or a.data_type != kind:
        return None
    out = [0] * (len(me.vertices) * width) if kind == 'INT' else [0.0] * (len(me.vertices) * width)
    a.data.foreach_get("vector" if kind == 'FLOAT_VECTOR' else "value", out)
    return out


def _point_attr_float2(me, name):
    a = me.attributes.get(name)
    if a is None or a.domain != 'POINT' or a.data_type != 'FLOAT2':
        return None
    out = [0.0] * (2 * len(me.vertices))
    a.data.foreach_get("vector", out)
    return out


def _vertex_data(obj, me, to_arm, scale, node_index, default_node):
    """Per Blender vertex: LithTech position, normal, UVs (first corner),
    and weights [(node, w)]. Normals come from lt_normal while the corners
    still have the normals they were imported with."""
    normal_mat = to_arm.to_3x3().inverted_safe().transposed()
    nv = len(me.vertices)
    stored = _point_attr(me, "lt_normal", 'FLOAT_VECTOR', 3) if export_abc._is_identity(to_arm) else None
    imp = me.attributes.get("lt_import_normal")
    unchanged = [False] * nv
    corner_n = [None] * nv
    if True:
        now = [0.0] * (3 * len(me.loops))
        me.corner_normals.foreach_get("vector", now)
        was = None
        if stored is not None and imp is not None and imp.domain == 'CORNER' and len(imp.data) == len(me.loops):
            was = [0.0] * (3 * len(me.loops))
            imp.data.foreach_get("vector", was)
        votes = [0] * nv
        sums = [Vector((0.0, 0.0, 0.0)) for _ in range(nv)]
        for li, loop in enumerate(me.loops):
            vi = loop.vertex_index
            n = Vector(now[3 * li:3 * li + 3])
            sums[vi] += n
            if was is not None:
                same = max(abs(was[3 * li + c] - now[3 * li + c]) for c in range(3)) < 1e-4
                votes[vi] += 1 if same else -(1 << 20)
        for vi in range(nv):
            unchanged[vi] = was is not None and votes[vi] >= 0
            corner_n[vi] = sums[vi]
    uv_layers = list(me.uv_layers)
    uvs = [[None] * len(uv_layers) for _ in range(nv)]
    seen = [False] * nv
    for s_ in range(len(uv_layers)):
        a = _point_attr_float2(me, "lt_uv%d" % s_)
        if a is not None:
            for vi in range(nv):
                uvs[vi] = list(uvs[vi])
                uvs[vi][s_] = (a[2 * vi], a[2 * vi + 1])
    for li, loop in enumerate(me.loops):
        vi = loop.vertex_index
        if seen[vi]:
            continue
        seen[vi] = True
        now_uv = [(l.uv[li].vector[0], 1.0 - l.uv[li].vector[1]) for l in uv_layers]
        # the stored value while the corner still has it (to float32 noise)
        uvs[vi] = [old if old is not None and abs(old[0] - n[0]) < 1e-6 and abs(old[1] - n[1]) < 1e-6 else n
                   for old, n in zip(list(uvs[vi]) + [None] * (len(now_uv) - len(uvs[vi])), now_uv)]
    group_names = {g.index: g.name for g in obj.vertex_groups}
    out = []
    for vi, v in enumerate(me.vertices):
        pos = convert.vec_from_blender(to_arm @ v.co, scale)
        if unchanged[vi] and stored is not None:
            n = tuple(stored[3 * vi:3 * vi + 3])
        else:
            nb = corner_n[vi] if corner_n[vi].length > 0 else Vector(v.normal)
            n = convert.normal_from_blender((normal_mat @ nb).normalized())
        ws = []
        for g in v.groups:
            ni = node_index.get(group_names.get(g.group))
            if ni is not None and g.weight > 0.0:
                ws.append((ni, g.weight))
        out.append((pos, n, [u if u is not None else (0.0, 0.0) for u in uvs[vi]], ws))
    return out


def _used_nodes_terminal(bones, nodes):
    """CalcUsedNodes: the bones none of whose children are also used."""
    children = {i: set() for i in range(len(nodes))}
    idx = {id(n): i for i, n in enumerate(nodes)}
    for i, n in enumerate(nodes):
        for c in n.children:
            children[i].add(idx[id(c)])
    used = set(b for b in bones if b != 0xFF)
    return [b for b in sorted(used) if not (children.get(b, set()) & used)]


def _mesh_kept(obj, me, vdata, nodes, node_index):
    """The imported layout, when the mesh still fits it; else None."""
    kind = obj.get("lt_mesh_kind")
    if kind is None or int(obj.get("lt_vertex_count", -1)) != len(me.vertices):
        return None
    deg = json.loads(obj.get("lt_degenerate_faces", "[]"))
    if int(obj.get("lt_face_count", -1)) != len(me.polygons) + len(deg):
        return None
    if any(len(p.vertices) != 3 for p in me.polygons):
        return None
    ro = ltb.RenderObject(int(kind))
    ro.stream_flags = [int(x) for x in obj.get("lt_stream_flags", [0x13, 0, 0, 0])]
    ro.max_bones_per_tri = int(obj.get("lt_max_bones_per_tri", 1))
    ro.max_bones_per_vert = int(obj.get("lt_max_bones_per_vert", 1))
    ro.basis_floats = int(obj.get("lt_basis_floats", 6))
    if ro.kind == ltb.NULL_MESH:
        return ro
    eff = obj.get("lt_bone_effector")
    if eff is not None:
        ro.bone_effector = node_index.get(eff, 0)
    if ro.kind == ltb.SKEL_MESH:
        f = list(obj.get("lt_skel_flags", [0, 0, 0, 0]))
        ro.reindexed_bones_flag, ro.use_matrix_palettes, ro.min_bone, ro.max_bone = (int(x) for x in f)
        ro.reindexed_bones = [int(x) for x in obj.get("lt_reindexed_bones", [])]
        ro.bone_sets = [ltb.BoneSet(a, b, bytes(c), d) for a, b, c, d in json.loads(obj.get("lt_bone_sets", "[]"))]
        if "lt_raw_vertex_data" in obj:
            ro.raw_vertex_data = bytes.fromhex(obj["lt_raw_vertex_data"])
    if ro.kind == ltb.VA_MESH:
        ro.undup_vertex_count, ro.anim_node_index = (int(x) for x in obj.get("lt_va", [len(me.vertices), 0]))
        dm = list(obj.get("lt_dup_map", []))
        ro.dup_map = [(dm[i], dm[i + 1]) for i in range(0, len(dm), 2)]
    # Faces back in their order, degenerate ones where they were.
    tris = [tuple(p.vertices) for p in me.polygons]
    for d in deg:
        tris.insert(d[0], tuple(d[1:]))
    ro.indices = [i for t in tris for i in (t[0], t[2], t[1])]
    blend = _point_attr(me, "lt_blend_weights", 'FLOAT_VECTOR', 3)
    pal = _point_attr(me, "lt_bone_indices", 'INT', 1)
    basis = _point_attr(me, "lt_basis", 'FLOAT_VECTOR', 3)
    basis2 = _point_attr(me, "lt_basis2", 'FLOAT_VECTOR', 3)
    nw, has_index = ro.blend_layout()
    # For each vertex, the bone set it belongs to.
    set_of = {}
    for bs in ro.bone_sets:
        for vi in range(bs.first_vertex, bs.first_vertex + bs.vertex_count):
            set_of[vi] = bs
    flags0 = ro.stream_flags[0] if ro.stream_flags else 0
    nuv = max((ltb.uv_sets(f) for f in ro.stream_flags), default=0)
    for vi, (pos, n, uvs, ws) in enumerate(vdata):
        v = ltb.Vertex(position=pos, normal=n, uvs=list(uvs[:nuv]) + [(0.0, 0.0)] * (nuv - len(uvs)))
        if nw:
            stored = tuple(blend[3 * vi:3 * vi + nw]) if blend else (0.0,) * nw
            bs = set_of.get(vi)
            if bs is not None and not ro.use_matrix_palettes:
                bones = []
                for b in bs.bones:
                    if b == 0xFF:
                        break
                    bones.append(b)
                k = len(bones)
                want = dict(ws)
                # the weights the stored values give, against Blender's
                rest = 1.0
                dec = []
                for i, b in enumerate(bones):
                    w = stored[i] if i < k - 1 and i < len(stored) else rest
                    if i < k - 1:
                        rest -= w
                    dec.append((b, w))
                if all(abs(want.get(b, 0.0) - w) < 1e-5 for b, w in dec) and \
                        all(b in dict(dec) for b in want if want[b] > 1e-5):
                    v.weights = stored
                else:
                    betas = list(stored)
                    for i in range(min(k - 1, nw)):
                        betas[i] = want.get(bones[i], 0.0)
                    v.weights = tuple(betas)
            else:
                v.weights = stored
        if has_index:
            v.bone_indices = int(pal[vi] if pal else 0).to_bytes(4, "little", signed=True)
        if flags0 & ltb.VDT_BASISVECTORS or any(f & ltb.VDT_BASISVECTORS for f in ro.stream_flags):
            b1 = tuple(basis[3 * vi:3 * vi + 3]) if basis else (0.0, 0.0, 0.0)
            b2 = tuple(basis2[3 * vi:3 * vi + 3]) if basis2 else (0.0, 0.0, 0.0)
            v.basis = b1 + b2 if ro.basis_floats == 6 else b1
        ro.vertices.append(v)
    ro.vertex_count = len(ro.vertices)
    if ro.raw_vertex_data is not None:
        ro.vertices = []
    ro.poly_count = len(ro.indices) // 3
    return ro


def _mesh_new(obj, me, vdata, nodes, default_node):
    """A fresh D3D layout for a mesh made or reshaped in Blender."""
    me.calc_loop_triangles()
    uv_layers = list(me.uv_layers)
    normal_of = {}
    # corners: (vertex, uv set values, normal)
    corner_key = {}
    keys = []
    tris = []
    now = [0.0] * (3 * len(me.loops))
    me.corner_normals.foreach_get("vector", now)
    for tri in me.loop_triangles:
        ks = []
        for li, vi in zip(tri.loops, tri.vertices):
            uv = tuple(round(c, 6) for l in uv_layers for c in (l.uv[li].vector[0], 1.0 - l.uv[li].vector[1]))
            n = vdata[vi][1]
            key = (vi, uv)
            if key not in corner_key:
                corner_key[key] = len(keys)
                keys.append((vi, [(uv[2 * i], uv[2 * i + 1]) for i in range(len(uv_layers))], n))
            ks.append(corner_key[key])
        tris.append((ks[0], ks[2], ks[1]))   # LithTech's winding
    # weights: the strongest four per vertex, normalised
    vw = []
    for vi, (pos, n, uvs, ws) in enumerate(vdata):
        ws = sorted(ws, key=lambda x: -x[1])[:4] or [(default_node, 1.0)]
        tot = sum(w for _, w in ws) or 1.0
        vw.append([(b, w / tot) for b, w in ws])
    bones_used = {b for ws in vw for b, _ in ws}
    nuv = len(uv_layers)
    flags = ltb.VDT_POSITION | ltb.VDT_NORMAL | ([0, ltb.VDT_UVSETS_1, ltb.VDT_UVSETS_2, ltb.VDT_UVSETS_3,
                                                  ltb.VDT_UVSETS_4][min(nuv, 4)])
    if len(bones_used) <= 1:
        ro = ltb.RenderObject(ltb.RIGID_MESH)
        ro.bone_effector = next(iter(bones_used)) if bones_used else default_node
        ro.stream_flags = [flags, 0, 0, 0]
        for vi, uvs, n in keys:
            ro.vertices.append(ltb.Vertex(position=vdata[vi][0], normal=n, uvs=uvs))
        ro.indices = [i for t in tris for i in t]
        ro.vertex_count = len(ro.vertices)
        ro.poly_count = len(tris)
        return ro, [ro.bone_effector]

    # Skeletal: pack triangles into bone sets of up to four bones.
    def tri_bones(t):
        score = {}
        for k in t:
            for b, w in vw[keys[k][0]]:
                score[b] = score.get(b, 0.0) + w
        return [b for b, _ in sorted(score.items(), key=lambda x: -x[1])[:4]]

    order = sorted(range(len(tris)), key=lambda i: sorted(tri_bones(tris[i])))
    sets = []   # [bones, [tri indices]]
    for ti in order:
        tb = tri_bones(tris[ti])
        if sets and len(set(sets[-1][0]) | set(tb)) <= 4:
            for b in tb:
                if b not in sets[-1][0]:
                    sets[-1][0].append(b)
            sets[-1][1].append(ti)
        else:
            sets.append([list(tb), [ti]])
    max_bones = max(len(s[0]) for s in sets)
    ro = ltb.RenderObject(ltb.SKEL_MESH)
    ro.max_bones_per_tri = max_bones
    ro.max_bones_per_vert = max(len(w) for w in vw)
    ro.stream_flags = [flags, 0, 0, 0]
    nw = max_bones - 1
    used = set()
    for bones, tis in sets:
        local = {}
        first = len(ro.vertices)
        for ti in tis:
            corners = []
            for k in tris[ti]:
                if k not in local:
                    local[k] = len(ro.vertices)
                    vi, uvs, n = keys[k]
                    want = dict(vw[vi])
                    # weights over this set's bones; the last is implicit
                    kept = [want.get(b, 0.0) for b in bones]
                    tot = sum(kept) or 1.0
                    kept = [w / tot for w in kept]
                    betas = tuple(kept[:len(bones) - 1]) + (0.0,) * (nw - (len(bones) - 1))
                    ro.vertices.append(ltb.Vertex(position=vdata[vi][0], weights=betas, normal=n, uvs=uvs))
                corners.append(local[k])
            ro.indices += corners
        used |= set(bones)
        ro.bone_sets.append(ltb.BoneSet(first, len(ro.vertices) - first,
                                        bytes(bones + [0xFF] * (4 - len(bones))), len(ro.indices)))
    if len(ro.vertices) > 0xFFFF:
        raise ExportError("%s has more than 65535 vertices after splitting (a D3D limit)" % obj.name)
    ro.vertex_count = len(ro.vertices)
    ro.poly_count = len(ro.indices) // 3
    return ro, sorted(used)


def _vertex_frames(obj, ro, scale, anims):
    """Vertex animation frames from shape keys "<animation>|<key>"."""
    keys = obj.data.shape_keys
    out = {}
    if keys is None:
        return out
    to_arm_scale = scale
    counts = json.loads(obj.get("lt_vertex_frame_counts", "{}"))
    for kb in keys.key_blocks:
        if "|" not in kb.name:
            continue
        an, _, k = kb.name.rpartition("|")
        try:
            k = int(k)
        except ValueError:
            continue
        n = int(counts.get(kb.name, ro.undup_vertex_count or len(kb.data)))
        fr = []
        for i in range(min(n, len(kb.data))):
            fr += convert.vec_from_blender(kb.data[i].co, to_arm_scale)
        out.setdefault(an, {})[k] = fr
    return out


# --------------------------------------------------------------------- #
#  Animations
# --------------------------------------------------------------------- #

def _channel(values, quant, full=False):
    """The keys a REL channel keeps: one if it never changes, else one per
    keyframe (shipped files never keep none). Floats that went through
    Blender count as unchanged within 1e-5."""
    if quant:
        values = [tuple(max(-32768, min(32767, int(round(c * quant)))) for c in v) for v in values]
        same = all(v == values[0] for v in values)
    else:
        same = all(max(abs(a - b) for a, b in zip(v, values[0])) < 1e-5 for v in values)
    if not full and same:
        return [values[0]]
    return values


def _encode_track(per_key, compression, full_pos=False, full_rot=False):
    t = ltb.NodeTrack()
    pos = [tuple(p) for p, q in per_key]
    rot = []
    prev = None
    for p, q in per_key:
        q = tuple(q)
        if prev is not None and sum(a * b for a, b in zip(prev, q)) < 0:
            q = tuple(-c for c in q)
        rot.append(q)
        prev = q
    if compression == ltb.ANCMPRS_NONE:
        t.positions, t.rotations = pos, rot
        return t
    t.positions = _channel(pos, ltb.POS16_SCALE if compression == ltb.ANCMPRS_REL_16 else None, full_pos)
    q16 = compression in (ltb.ANCMPRS_REL_16, ltb.ANCMPRS_REL_16_ROT_ONLY)
    t.rotations = _channel(rot, ltb.QUAT16_SCALE if q16 else None, full_rot)
    return t


def _encode_timed_track(per_key, compression, keyframes, stored):
    """Versions 18 and 20: keys with their times. The stored times when
    the action still has those keys, else every keyframe's."""
    full = _encode_track(per_key, compression, True, True)
    t = ltb.NodeTrack()
    times = [k.time for k in keyframes]
    for which, vals in ((0, full.positions), (1, full.rotations)):
        want = stored[which] if stored else None
        if want is None:
            if len(set(map(tuple, vals))) == 1:
                want = times[:1]
            else:
                want = times
        out_v, out_t = [], []
        seen = {}
        for tm in want:
            # the nth key at a time is the nth keyframe at that time
            n = seen.get(tm, 0)
            seen[tm] = n + 1
            at = [j for j, x in enumerate(times) if x == tm]
            i = at[min(n, len(at) - 1)] if at else min(range(len(times)), key=lambda j: abs(times[j] - tm))
            out_v.append(vals[i])
            out_t.append(tm)
        if which == 0:
            t.positions, t.pos_times = out_v, out_t
        else:
            t.rotations, t.rot_times = out_v, out_t
    return t


# --------------------------------------------------------------------- #

def save(context, filepath, *, armature=None, scale=1.0, use_selection=False,
         export_animations=True, version=None):
    arm_obj = armature or export_abc.find_armature(context)
    if arm_obj is None:
        raise ExportError(export_abc.nothing_to_export(context))
    if context.object and context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    fps = context.scene.render.fps / context.scene.render.fps_base

    abc_nodes, node_bones, abc_root = export_abc._build_skeleton(arm_obj, scale)
    root = _to_ltb_node(abc_root)
    m = ltb.Model(root=root)
    nodes = m.nodes
    for n, b in zip(nodes, node_bones):
        if b is not None and "lt_bind" in b:
            kept = tuple(float(x) for x in b["lt_bind"])
            if max(abs(x - y) for x, y in zip(kept, n.bind_matrix)) < 1e-2:
                n.bind_matrix = kept
    node_index = {b.name: i for i, b in enumerate(node_bones) if b is not None}

    stored_version = int(arm_obj.get("lt_version", 23)) if arm_obj.get("lt_format") == "ltb" else 23
    m.version = int(version) if version else stored_version
    if "lt_ltb_header" in arm_obj and arm_obj.get("lt_format") == "ltb":
        m.header = bytes(list(arm_obj["lt_ltb_header"]))
    m.command_string = arm_obj.get("lt_command_string", "")
    m.radius = float(arm_obj.get("lt_internal_radius", 96.0))
    L = ltb.layout(m.version)
    if "lt_obbs_json" in arm_obj and m.version == stored_version:
        m.obbs = [tuple(o) for o in json.loads(arm_obj["lt_obbs_json"])]

    # Pieces and LODs.
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

    extra = {}
    for o in meshes:
        li = int(o.get("lt_lod", 0))
        if li > 0:
            extra[(piece_key(o), li)] = o

    va_meshes = []   # (object, render object)
    for o in base:
        default_node = node_index.get(o.parent_bone, 0) if o.parent_type == 'BONE' else 0
        p = ltb.Piece(o.get("lt_piece", o.name))
        offs = list(o.get("lt_lod_offsets", [0, 0]))
        p.min_lod_offset, p.max_lod_offset = int(offs[0]), int(offs[1])
        nlods = int(o.get("lt_lod_count", 1))
        objs = [o] + [extra.get((piece_key(o), li)) for li in range(1, max(nlods, 1 + max(
            [k[1] for k in extra if k[0] == piece_key(o)] or [0])))]
        for li, lo in enumerate(objs):
            if lo is None:
                continue
            me = export_abc._evaluated_mesh(lo, depsgraph)
            try:
                to_arm = export_abc._to_armature(lo, arm_obj, arm_inv)
                vdata = _vertex_data(lo, me, to_arm, scale, node_index, default_node)
                ro = _mesh_kept(lo, me, vdata, nodes, node_index) if export_abc._is_identity(to_arm) else None
                if ro is not None:
                    used = list(lo.get("lt_used_nodes", []))
                    if not used:
                        bones = [b for bs in ro.bone_sets for b in bs.bones] if ro.kind == ltb.SKEL_MESH \
                            else ([ro.bone_effector] if ro.kind in (ltb.RIGID_MESH, ltb.VA_MESH) else [])
                        used = _used_nodes_terminal(bones, nodes) if ro.kind == ltb.SKEL_MESH else bones
                else:
                    ro, bones = _mesh_new(lo, me, vdata, nodes, default_node)
                    used = _used_nodes_terminal(bones, nodes) if ro.kind == ltb.SKEL_MESH else bones
            finally:
                bpy.data.meshes.remove(me)
            lod = ltb.LOD()
            lod.texture_count = int(lo.get("lt_texture_count", 1))
            lod.textures = [int(x) for x in lo.get("lt_textures", [0, -1, -1, -1])]
            if "lt_textures" not in lo:
                mat = lo.active_material
                slot = int(mat.get("lt_texture_index", 0)) if mat is not None else 0
                lod.textures = [slot, -1, -1, -1]
            lod.render_style = int(lo.get("lt_render_style", 0))
            lod.render_priority = int(lo.get("lt_render_priority", 0))
            lod.mesh = ro
            lod.used_nodes = bytes(used) if L.used_nodes and ro.kind != ltb.NULL_MESH else b""
            if "lt_used_nodes" not in lo and ro.kind == ltb.VA_MESH:
                lod.used_nodes = b""
            if not L.used_nodes:
                lod.used_nodes = b""
            p.lods.append(lod)
            p.lod_distances.append(float(lo.get("lt_lod_distance", 0.0)))
            if ro.kind == ltb.VA_MESH:
                va_meshes.append((lo, ro))
        m.pieces.append(p)

    # Sockets.
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
        sc = tuple(e.get("lt_socket_scale", (1.0, 1.0, 1.0)))
        m.sockets.append(ltb.Socket(node_index[e.parent_bone], e.get("lt_name", e.name), q, pos, sc))

    # Weight sets.
    for ws in json.loads(arm_obj.get("lt_weight_sets_json", "[]")):
        w = ws.get("weights", {})
        m.weight_sets.append(ltb.WeightSet(ws["name"], [float(w.get(n.name, 0.0)) for n in nodes]))

    # Animations.
    rest_locals = []
    for b in node_bones:
        if b is None:
            rest_locals.append(Matrix.Identity(4))
        elif b.parent is None:
            rest_locals.append(b.matrix_local.copy())
        else:
            rest_locals.append(b.parent.matrix_local.inverted() @ b.matrix_local)
    translations = []
    if export_animations:
        for a in export_abc._actions_for(arm_obj, node_bones):
            anim, tr = export_abc._animation_from_action(a, arm_obj, abc_nodes, node_bones, rest_locals, scale, fps)
            comp = int(a.get("lt_compression", ltb.ANCMPRS_REL_16_ROT_ONLY))
            vnodes = set(a.get("lt_vertex_anim_nodes", []))
            if vnodes:
                comp = ltb.ANCMPRS_NONE
            keys = [ltb.Keyframe(k.time, k.command) for k in anim.keyframes]
            stored_times = list(a.get("lt_key_times", []))
            repeats = {int(k): v for k, v in json.loads(a.get("lt_repeated_keys_json", "{}")).items()}
            if stored_times and sorted(set(stored_times)) == [k.time for k in keys] and repeats:
                # keys repeating a time: back in their places
                by_index = dict((i, c) for i, c in json.loads(a.get("lt_key_strings_by_index", "[]")))
                at = {k.time: i for i, k in enumerate(keys)}
                new_tr = [[] for _ in anim.node_transforms]
                for i, t in enumerate(stored_times):
                    for ni in range(len(new_tr)):
                        if i in repeats and ni < len(repeats[i]):
                            p_, q_ = repeats[i][ni]
                            new_tr[ni].append((tuple(p_), tuple(q_)))
                        else:
                            new_tr[ni].append(anim.node_transforms[ni][at[t]])
                anim.node_transforms = new_tr
                keys = [ltb.Keyframe(t, by_index.get(i, "")) for i, t in enumerate(stored_times)]
            raw_rot = {int(k): v for k, v in json.loads(a.get("lt_raw_rotations_json", "{}")).items()}
            for ni, qs in raw_rot.items():
                per = anim.node_transforms[ni]
                if len(qs) == len(per):
                    out_ = []
                    for (p_, q_), r in zip(per, qs):
                        # what Blender made of the stored value: the rotation
                        # part of the matrix it gives
                        mr = Matrix([ltb.quat_to_matrix((0.0, 0.0, 0.0), r)[4 * i:4 * i + 3] for i in range(3)])
                        try:
                            e = mr.to_4x4().decompose()[1]   # as the import made it
                            ex = (e.x, e.y, e.z, e.w)
                        except Exception:
                            ex = (0.0, 0.0, 0.0, 1.0)
                        same = abs(abs(sum(x * y for x, y in zip(ex, q_))) - 1.0) < 1e-4
                        out_.append((p_, tuple(r)) if same else (p_, q_))
                    anim.node_transforms[ni] = out_
            la = ltb.Animation(anim.name, tuple(anim.dims), comp, anim.interpolation_time, keys)
            counts = a.get("lt_channel_counts", "")
            if len(counts) != 2 * len(nodes):
                counts = ""
            stored_key_times = {int(k): v for k, v in json.loads(a.get("lt_key_times_json", "{}")).items()}
            for ni in range(len(nodes)):
                if ni in vnodes:
                    t = ltb.NodeTrack(is_vertex_anim=1)
                    frames = {}
                    for lo, ro in va_meshes:
                        if ro.anim_node_index == ni:
                            frames = _vertex_frames(lo, ro, scale, None).get(anim.name, {})
                    t.vertex_frames = [frames.get(k, []) for k in range(len(la.keyframes))]
                    la.tracks.append(t)
                elif L.rel_key_times and comp != ltb.ANCMPRS_NONE:
                    la.tracks.append(_encode_timed_track(anim.node_transforms[ni], comp, la.keyframes,
                                                         stored_key_times.get(ni)))
                else:
                    t = _encode_track(anim.node_transforms[ni], comp,
                                      counts[2 * ni:2 * ni + 1] == "f", counts[2 * ni + 1:2 * ni + 2] == "f")
                    # an imported channel keeps the number of keys it had
                    if counts[2 * ni:2 * ni + 1] in ("0", "1"):
                        t.positions = t.positions[:int(counts[2 * ni])]
                    if counts[2 * ni + 1:2 * ni + 2] in ("0", "1"):
                        t.rotations = t.rotations[:int(counts[2 * ni + 1])]
                    la.tracks.append(t)
            m.animations.append(la)
            translations.append(tr)
    if not m.animations:
        b = export_abc._bind_animation(abc_nodes, "base", (24.0, 53.0, 24.0))
        la = ltb.Animation("base", b.dims, ltb.ANCMPRS_REL_16_ROT_ONLY, 200, [ltb.Keyframe(0, "")])
        la.tracks = [_encode_track(b.node_transforms[ni], la.compression) for ni in range(len(nodes))]
        m.animations.append(la)
        translations.append((0.0, 0.0, 0.0))

    # Child models and animation bindings (the model itself first).
    stored = json.loads(arm_obj.get("lt_child_models_json", "[]")) if arm_obj.get("lt_format") == "ltb" else []
    m.child_models = [c["name"] for c in stored]
    m.anim_bindings = [[ltb.AnimBinding(a.name, a.dims, tr) for a, tr in zip(m.animations, translations)]]
    for c in stored:
        m.anim_bindings.append([ltb.AnimBinding(b[0], tuple(b[1]), tuple(b[2])) for b in c.get("anim_bindings", [])])
    if L.child_relations:
        rel = json.loads(arm_obj.get("lt_child_relations_json", "[]"))
        ident = [((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))] * len(nodes)
        m.child_relations = []
        for i in range(1 + len(m.child_models)):
            if i < len(rel) and len(rel[i][1]) == len(nodes):
                m.child_relations.append((rel[i][0], [(tuple(p), tuple(q)) for p, q in rel[i][1]]))
            else:
                m.child_relations.append((0, ident))

    # Allocation counts: the ones the engine sizes memory by, computed;
    # the hints, no smaller than what the imported file had.
    counts = ltb.compute_alloc_counts(m)
    stored_counts = list(arm_obj.get("lt_alloc_counts", [])) if arm_obj.get("lt_format") == "ltb" else []
    for i in range(len(counts)):
        if i not in ltb.ALLOC_EXACT and i < len(stored_counts):
            counts[i] = max(counts[i], int(stored_counts[i]))
    m.alloc_counts = counts[:L.alloc_count]

    ltb.write_file(m, filepath)
    return m, [filepath]
