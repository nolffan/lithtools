"""Import a LithTech .ltb model (NOLF2, Contract J.A.C.K., Tron 2.0).

The same layout as import_abc (armature, a mesh per piece, hidden LOD
objects, socket empties, actions), with what .ltb adds:

  * each LOD is a D3D render object: its vertices become the mesh's
    vertices in their stored order (so UV seams are split vertices, as the
    game draws them) and its triangles the faces
  * skeletal meshes weight their vertices through bone sets (up to four
    nodes for a run of vertices); the weights go into vertex groups, and
    the sets themselves into lt_bone_sets so an unchanged mesh exports as
    it was
  * rigid and vertex-animated meshes follow one node (lt_bone_effector)
  * vertex animation frames become shape keys named "<animation>|<key>"
  * per LOD: lt_textures, lt_texture_count, lt_render_style,
    lt_render_priority, lt_lod_distance, lt_mesh_kind
  * per animation: lt_compression (0 none, 1 relevant keys, 2 16-bit,
    3 16-bit rotations)
"""

import json
import math
import os

import bpy
from mathutils import Matrix, Vector

from .lithtech import ltb
from . import butes
from . import convert
from . import import_abc
from . import textures


class _Skeleton:
    """What import_abc's armature code reads from a model."""

    def __init__(self, m):
        self.nodes = m.nodes
        self._parents = m.node_parents()
        self.internal_radius = m.radius

    def node_parents(self):
        return self._parents


class _Keyed:
    """An ltb animation as import_abc._create_action reads one: keyframes
    and decoded (position, rotation) per node per key. A key repeating an
    earlier key's time (an F-curve can't key a frame twice) is left out
    here and kept in `repeats` (key index -> per node (position, rotation)),
    for the action's lt_repeated_keys_json."""

    def __init__(self, anim, nnodes):
        self.keyframes = []
        keep = []
        self.repeats = {}
        seen = set()
        for i, k in enumerate(anim.keyframes):
            if k.time in seen:
                self.repeats[i] = [[list(p), list(q)] for p, q in
                                   (anim.key_transform(n, i) for n in range(nnodes))]
                continue
            seen.add(k.time)
            keep.append(i)
            self.keyframes.append(k)
        self.node_transforms = [[anim.key_transform(n, i) for i in keep] for n in range(nnodes)]


def _near_constant(values):
    """Whether export_ltb would keep a channel as one key (so a whole
    channel has to be marked to stay whole)."""
    return all(max(abs(a - b) for a, b in zip(v, values[0])) < 1e-5 for v in values)


def vertex_weights(mesh):
    """[(node, weight), ...] per vertex of a render object, as the D3D
    renderer blends them."""
    out = [[] for _ in mesh.vertices]
    if mesh.kind in (ltb.RIGID_MESH, ltb.VA_MESH):
        for w in out:
            w.append((mesh.bone_effector, 1.0))
        return out
    if mesh.kind != ltb.SKEL_MESH:
        return out
    if mesh.use_matrix_palettes:
        nb = mesh.max_bones_per_vert
        for vi, v in enumerate(mesh.vertices):
            idx = v.bone_indices or b"\0\0\0\0"
            rest = 1.0
            for i in range(nb):
                b = idx[i]
                node = mesh.reindexed_bones[b] if mesh.reindexed_bones_flag and b < len(mesh.reindexed_bones) else b
                w = v.weights[i] if i < nb - 1 else rest
                rest -= w if i < nb - 1 else 0.0
                out[vi].append((node, w))
        return out
    for bs in mesh.bone_sets:
        bones = []
        for b in bs.bones:
            if b == 0xFF:
                break   # SetTransformsToBoneSet stops at the first unused slot
            bones.append(b)
        k = len(bones)
        for vi in range(bs.first_vertex, min(bs.first_vertex + bs.vertex_count, len(mesh.vertices))):
            v = mesh.vertices[vi]
            rest = 1.0
            for i, b in enumerate(bones):
                if i < k - 1:
                    w = v.weights[i] if i < len(v.weights) else 0.0
                    rest -= w
                else:
                    w = rest
                out[vi].append((b, w))
    return out


def _create_lod_object(name, lod, scale, bone_names, model_name):
    mesh = lod.mesh
    me = bpy.data.meshes.new(name)
    if mesh.raw_vertex_data is not None and mesh.vertex_count:
        # Vertices of unknown layout (kept as bytes for export): place them
        # by their first three floats.
        import struct
        size = len(mesh.raw_vertex_data) // mesh.vertex_count
        mesh.vertices = [ltb.Vertex(position=struct.unpack_from("<3f", mesh.raw_vertex_data, i * size)
                                    if size >= 12 else (0.0, 0.0, 0.0))
                         for i in range(mesh.vertex_count)]
    verts = [convert.vec_to_blender(v.position or (0.0, 0.0, 0.0), scale) for v in mesh.vertices]
    idx = mesh.indices
    # The swap of axes turns the winding round: corners go in as 0, 2, 1.
    faces = [(idx[i], idx[i + 2], idx[i + 1]) for i in range(0, len(idx) - 2, 3)]
    # Blender can't hold a degenerate triangle; keep them aside for export.
    nv = len(verts)

    def usable(f):
        return len(set(f)) == 3 and max(f) < nv

    good = [f for f in faces if usable(f)]
    me.from_pydata(verts, [], good)
    bad = [[i] + list(f) for i, f in enumerate(faces) if not usable(f)]

    nsets = max((len(v.uvs) for v in mesh.vertices), default=0)
    for s in range(nsets):
        uv = me.uv_layers.new(name="UVMap" if s == 0 else "UVMap.%d" % s)
        flat = []
        for poly in me.polygons:
            for li in poly.loop_indices:
                v = mesh.vertices[me.loops[li].vertex_index]
                u, vv = v.uvs[s] if s < len(v.uvs) else (0.0, 0.0)
                flat += (u, 1.0 - vv)
        uv.data.foreach_set("uv", flat)

    if nsets:
        # Per vertex as stored (a vertex no face uses has no corners to
        # carry its UV).
        for s_ in range(nsets):
            a = me.attributes.new("lt_uv%d" % s_, 'FLOAT2', 'POINT')
            a.data.foreach_set("vector", [c for v in mesh.vertices for c in (v.uvs[s_] if s_ < len(v.uvs) else (0.0, 0.0))])

    me.shade_smooth()
    normals = [v.normal or (0.0, 0.0, 1.0) for v in mesh.vertices]
    nattr = me.attributes.new("lt_normal", 'FLOAT_VECTOR', 'POINT')
    nattr.data.foreach_set("vector", [c for n in normals for c in n])
    bn = [convert.normal_to_blender(n) for n in normals]
    me.normals_split_custom_set_from_vertices([n.normalized() if n.length else n for n in bn])
    me.update()
    cn = [0.0] * (3 * len(me.loops))
    me.corner_normals.foreach_get("vector", cn)
    me.attributes.new("lt_import_normal", 'FLOAT_VECTOR', 'CORNER').data.foreach_set("vector", cn)
    if any(v.basis for v in mesh.vertices):
        b = me.attributes.new("lt_basis", 'FLOAT_VECTOR', 'POINT')
        b.data.foreach_set("vector", [c for v in mesh.vertices for c in (v.basis or (0.0,) * 6)[:3]])
        if mesh.basis_floats == 6:
            b2 = me.attributes.new("lt_basis2", 'FLOAT_VECTOR', 'POINT')
            b2.data.foreach_set("vector", [c for v in mesh.vertices for c in (v.basis or (0.0,) * 6)[3:6]])

    if any(v.weights for v in mesh.vertices):
        # The blend weights as stored (slots a bone set doesn't use hold
        # leftovers), for an unchanged vertex to export exactly.
        w = me.attributes.new("lt_blend_weights", 'FLOAT_VECTOR', 'POINT')
        w.data.foreach_set("vector", [c for v in mesh.vertices for c in (tuple(v.weights) + (0.0,) * 3)[:3]])

    obj = bpy.data.objects.new(name, me)
    if bad:
        # (index in the triangle list, three corners in Blender's winding)
        obj["lt_degenerate_faces"] = json.dumps(bad)
    groups = [obj.vertex_groups.new(name=bn_) for bn_ in bone_names]
    batches = {}
    for vi, ws in enumerate(vertex_weights(mesh)):
        for node, w in ws:
            if 0 <= node < len(groups):
                batches.setdefault((node, w), []).append(vi)
    for (node, w), vis in batches.items():
        groups[node].add(vis, max(0.0, min(1.0, w)), 'REPLACE')

    # What the mesh's D3D layout was, for an unchanged mesh to export as it
    # was (export_ltb).
    obj["lt_mesh_kind"] = mesh.kind
    obj["lt_vertex_count"] = len(mesh.vertices)
    obj["lt_face_count"] = len(faces)
    obj["lt_stream_flags"] = list(mesh.stream_flags)
    obj["lt_max_bones_per_tri"] = mesh.max_bones_per_tri
    obj["lt_max_bones_per_vert"] = mesh.max_bones_per_vert
    if mesh.kind in (ltb.RIGID_MESH, ltb.VA_MESH):
        obj["lt_bone_effector"] = bone_names[mesh.bone_effector] if mesh.bone_effector < len(bone_names) else ""
    if mesh.kind == ltb.SKEL_MESH:
        obj["lt_skel_flags"] = [mesh.reindexed_bones_flag, mesh.use_matrix_palettes, mesh.min_bone, mesh.max_bone]
        if mesh.reindexed_bones:
            obj["lt_reindexed_bones"] = list(mesh.reindexed_bones)
        obj["lt_bone_sets"] = json.dumps([[b.first_vertex, b.vertex_count, list(b.bones), b.index_into_index_buffer]
                                          for b in mesh.bone_sets])
        if mesh.use_matrix_palettes:
            a = me.attributes.new("lt_bone_indices", 'INT', 'POINT')
            a.data.foreach_set("value", [int.from_bytes(v.bone_indices or b"\0\0\0\0", "little", signed=True)
                                         for v in mesh.vertices])
        if mesh.raw_vertex_data is not None:
            obj["lt_raw_vertex_data"] = mesh.raw_vertex_data.hex()
    if mesh.kind == ltb.VA_MESH:
        obj["lt_va"] = [mesh.undup_vertex_count, mesh.anim_node_index]
        obj["lt_dup_map"] = [c for d in mesh.dup_map for c in d]
    if mesh.basis_floats != 6:
        obj["lt_basis_floats"] = mesh.basis_floats
    if lod.used_nodes:
        obj["lt_used_nodes"] = list(lod.used_nodes)
    return obj


def _add_vertex_animation(obj, mesh, m, scale):
    """Shape keys for a vertex-animated mesh: one per keyframe of each
    animation whose track for the mesh's node holds vertex frames."""
    node = mesh.anim_node_index
    made = False
    counts = {}
    for a in m.animations:
        if node >= len(a.tracks) or not a.tracks[node].is_vertex_anim:
            continue
        for k, fr in enumerate(a.tracks[node].vertex_frames):
            if not made:
                obj.shape_key_add(name="Basis", from_mix=False)
                made = True
            sk = obj.shape_key_add(name="%s|%d" % (a.name, k), from_mix=False)
            sk.value = 0.0
            n = min(len(fr) // 3, mesh.undup_vertex_count, len(sk.data))
            pts = {}
            for i in range(n):
                pts[i] = convert.vec_to_blender(fr[3 * i:3 * i + 3], scale)
            # duplicated vertices follow their source (DupMap)
            for src, dst in mesh.dup_map:
                if src in pts and dst < len(sk.data):
                    pts[dst] = pts[src]
            for i, p in pts.items():
                sk.data[i].co = p
            counts[sk.name] = len(fr) // 3
    if counts:
        # (shape keys can't hold custom properties)
        obj["lt_vertex_frame_counts"] = json.dumps(counts)


def load(context, filepath, *, scale=1.0, face_front=True, import_animations=True,
         child_animations=True, import_lods=True, import_sockets=True,
         import_textures=True, texture_dirs=()):
    m = ltb.read_file(filepath)
    if isinstance(m, ltb.ChunkedModel):
        raise ltb.LTBError("%s is a chunked .ltb from before the format settled; the games can't "
                           "load these and neither can this importer" % os.path.basename(filepath))
    name = os.path.splitext(os.path.basename(filepath))[0]
    fps = context.scene.render.fps / context.scene.render.fps_base

    if context.object and context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    collection = bpy.data.collections.new(name)
    context.scene.collection.children.link(collection)

    skel = _Skeleton(m)
    arm_obj, bone_names = import_abc._create_armature(context, collection, name, skel, scale, align_bones=True)
    nodes = m.nodes
    parents = skel.node_parents()
    if face_front:
        arm_obj.rotation_euler = (0.0, 0.0, math.pi)

    arm_obj["lt_format"] = "ltb"
    arm_obj["lt_version"] = m.version
    arm_obj["lt_ltb_header"] = list(m.header)
    arm_obj["lt_command_string"] = m.command_string
    arm_obj["lt_internal_radius"] = m.radius
    arm_obj["lt_scale"] = scale
    if m.alloc_counts is not None:
        arm_obj["lt_alloc_counts"] = list(m.alloc_counts)
    if m.obbs:
        arm_obj["lt_obbs_json"] = json.dumps([list(o) for o in m.obbs])
    arm_obj["lt_weight_sets_json"] = json.dumps(
        [{"name": ws.name, "weights": {nodes[i].name: w for i, w in enumerate(ws.weights) if i < len(nodes)}}
         for ws in m.weight_sets])
    arm_obj["lt_child_models_json"] = json.dumps(
        [{"name": c, "anim_bindings": [[b.name, list(b.dims), list(b.translation)]
                                       for b in (m.anim_bindings[i + 1] if i + 1 < len(m.anim_bindings) else [])]}
         for i, c in enumerate(m.child_models)])
    if m.child_relations:
        arm_obj["lt_child_relations_json"] = json.dumps(
            [[save, [[list(p), list(q)] for p, q in rel]] for save, rel in m.child_relations])

    # Bind matrices a bone can't hold exactly (scale, shear): kept as they
    # were, for export while the bone stays where it was imported.
    for i, n in enumerate(nodes):
        b = arm_obj.data.bones[bone_names[i]]
        back = convert.matrix_from_blender(convert.bone_matrix(b), 1.0 / scale)
        if max(abs(x - y) for x, y in zip(back, n.bind_matrix)) > 1e-5:
            b["lt_bind"] = list(n.bind_matrix)

    explicit = ()
    if import_textures:
        found = butes.model_skins(filepath)
        if found:
            # the game's own choice (ATTRIBUTES/MODELBUTES.TXT)
            arm_obj["lt_butes_model"] = found[0]
            if found[2]:
                arm_obj["lt_render_styles"] = list(found[2])
            explicit = found[1]

    images = {}
    materials = {}
    lod_collection = None
    slot_count = len({lod.textures[0] for p in m.pieces for lod in p.lods if lod.texture_count})
    for pi, p in enumerate(m.pieces):
        for li, lod in enumerate(p.lods):
            if li > 0 and not import_lods:
                break
            obj_name = p.name if li == 0 else "%s_LOD%d" % (p.name, li)
            obj = _create_lod_object(obj_name, lod, scale, bone_names, name)
            slot = lod.textures[0] if lod.texture_count else 0
            image = None
            if import_textures and lod.texture_count:
                if slot not in images:
                    images[slot] = textures.find_skin(filepath, slot, slot_count, texture_dirs, explicit)
                image = images[slot]
            obj.data.materials.append(import_abc._material_for(name, slot, image, materials))
            if lod.mesh.kind == ltb.VA_MESH:
                _add_vertex_animation(obj, lod.mesh, m, scale)
            if li == 0:
                collection.objects.link(obj)
            else:
                if lod_collection is None:
                    lod_collection = bpy.data.collections.new(name + " LODs")
                    collection.children.link(lod_collection)
                    lod_collection.hide_viewport = True
                    lod_collection.hide_render = True
                lod_collection.objects.link(obj)
            obj.parent = arm_obj
            mod = obj.modifiers.new("Armature", 'ARMATURE')
            mod.object = arm_obj
            obj["lt_piece"] = p.name
            obj["lt_piece_index"] = pi
            obj["lt_lod"] = li
            obj["lt_lod_distance"] = p.lod_distances[li] if li < len(p.lod_distances) else 0.0
            obj["lt_textures"] = list(lod.textures)
            obj["lt_texture_count"] = lod.texture_count
            obj["lt_render_style"] = lod.render_style
            obj["lt_render_priority"] = lod.render_priority
            if li == 0:
                obj["lt_lod_offsets"] = [p.min_lod_offset, p.max_lod_offset]
                obj["lt_lod_count"] = len(p.lods)

    if import_sockets:
        for si, s in enumerate(m.sockets):
            e = import_abc._create_socket(collection, arm_obj, bone_names, s, si, scale)
            if abs(sum(c * c for c in s.rotation) - 1.0) > 1e-4:
                # not a unit quaternion: kept, with the rotation Blender made of it
                e["lt_socket_rotation"] = list(s.rotation)
                e["lt_socket_basis_rotation"] = list(e.matrix_basis.to_quaternion())
            if tuple(s.scale) != (1.0, 1.0, 1.0):
                e["lt_socket_scale"] = list(s.scale)

    if import_animations:
        rest_inv = import_abc._rest_locals(arm_obj, bone_names, parents)
        own_map = [(bn, i) for i, bn in enumerate(bone_names)]
        first = None
        for ai, a in enumerate(m.animations):
            keyed = _Keyed(a, len(nodes))
            act = import_abc._create_action(arm_obj, a.name, keyed, own_map, rest_inv, scale, fps)
            if keyed.repeats:
                act["lt_repeated_keys_json"] = json.dumps(keyed.repeats)
            strings = [[k.time, k.command] for k in a.keyframes if k.command]
            if strings:
                act["lt_key_strings_json"] = json.dumps(strings)
            act["lt_model"] = name
            act["lt_index"] = ai
            act["lt_dims"] = list(a.dims)
            b = m.anim_bindings[0][ai] if m.anim_bindings and ai < len(m.anim_bindings[0]) else None
            act["lt_translation"] = list(b.translation) if b else [0.0, 0.0, 0.0]
            act["lt_interpolation_ms"] = a.interpolation_time
            act["lt_compression"] = a.compression
            nk = len(a.keyframes)
            if a.compression != ltb.ANCMPRS_NONE:
                # Channels kept whole although their keys are all the same
                # (the packer compared floats before quantising them), and
                # the key times versions 18 and 20 store, so they export as
                # they were.
                # How many keys each channel kept: "0" none, "1" one, "f" one
                # per keyframe (two characters a node: position, rotation).
                counts = []
                times = {}
                for ni, t in enumerate(a.tracks):
                    for vals in (t.positions, t.rotations):
                        counts.append("0" if not vals else "1" if len(vals) == 1 and nk != 1 else "f")
                    if t.pos_times or t.rot_times:
                        times[ni] = [list(t.pos_times), list(t.rot_times)]
                act["lt_channel_counts"] = "".join(counts)
                if times:
                    act["lt_key_times_json"] = json.dumps(times)
            raw = {}
            for ni in range(len(nodes)):
                qs = [a.key_transform(ni, k)[1] for k in range(nk)]
                if any(abs(sum(c * c for c in q) - 1.0) > 1e-3 for q in qs):
                    raw[ni] = [list(q) for q in qs]
            if raw:
                # rotations that aren't unit quaternions (Blender keeps unit
                # ones only), as stored
                act["lt_raw_rotations_json"] = json.dumps(raw)
            if len({k.time for k in a.keyframes}) != nk:
                act["lt_key_times"] = [k.time for k in a.keyframes]
                act["lt_key_strings_by_index"] = json.dumps(
                    [[i, k.command] for i, k in enumerate(a.keyframes) if k.command])
            if any(t.is_vertex_anim for t in a.tracks):
                act["lt_vertex_anim_nodes"] = [i for i, t in enumerate(a.tracks) if t.is_vertex_anim]
            first = first or act

        if child_animations:
            model_dir = os.path.dirname(filepath)
            for c in m.child_models:
                path = import_abc._find_file_nocase(model_dir, c)
                if not path:
                    print("LithTech: child model %r not found next to %s" % (c, filepath))
                    continue
                try:
                    cm = ltb.read_file(path)
                except ltb.LTBError as e:
                    print("LithTech: can't read child model %s: %s" % (path, e))
                    continue
                if not isinstance(cm, ltb.Model):
                    continue
                cnodes = cm.nodes
                cidx = {n.name.lower(): i for i, n in enumerate(cnodes)}
                cmap = [(bn, cidx.get(nodes[i].name.lower())) for i, bn in enumerate(bone_names)]
                for ai, a in enumerate(cm.animations):
                    act = import_abc._create_action(arm_obj, a.name, _Keyed(a, len(cnodes)), cmap,
                                                    rest_inv, scale, fps)
                    act["lt_model"] = name
                    act["lt_child"] = c
                    act["lt_index"] = ai
                    act["lt_dims"] = list(a.dims)
                    first = first or act
        if first is not None:
            arm_obj.animation_data.action = first

    for o in context.view_layer.objects:
        o.select_set(False)
    arm_obj.select_set(True)
    context.view_layer.objects.active = arm_obj
    return arm_obj
