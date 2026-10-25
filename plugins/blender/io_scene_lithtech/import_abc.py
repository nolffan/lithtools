"""Import a LithTech .abc model as an armature with a mesh per piece.

What goes where:
  * nodes          -> bones (props lt_index, lt_flags)
  * pieces         -> mesh objects parented to the armature with an Armature
                      modifier; vertex groups by node name hold the biases
                      (props lt_piece, lt_texture_index, lt_specular_*,
                      lt_lod_weight)
  * LODs 1..n      -> hidden mesh objects in a "<model> LODs" collection
                      (props lt_piece, lt_lod)
  * sockets        -> arrow empties parented to their node's bone (lt_socket)
  * animations     -> actions with fake users; keyframe strings become the
                      action's pose markers (props lt_dims, lt_translation,
                      lt_interpolation_ms, lt_weight_set, lt_index)
  * the rest       -> custom properties on the armature object: lt_version,
                      lt_command_string, lt_internal_radius, lt_lod_distances,
                      lt_scale and JSON for weight sets and child models
"""

import json
import math
import os

import bpy
from mathutils import Matrix, Vector

from .lithtech import abc
from . import convert
from . import textures


def _find_file_nocase(directory, name):
    """name in directory, ignoring case (the games ran on Windows)."""
    path = os.path.join(directory, name)
    if os.path.exists(path):
        return path
    want = name.replace("\\", "/").lower()
    parts = want.split("/")
    cur = directory
    for part in parts:
        try:
            match = next((e for e in os.listdir(cur) if e.lower() == part), None)
        except OSError:
            return None
        if match is None:
            return None
        cur = os.path.join(cur, match)
    return cur


def _bone_lengths(binds, parents, default):
    """A display length for each bone: the distance to its nearest child,
    within reason, else the default."""
    lengths = [default] * len(binds)
    for i, p in enumerate(parents):
        if p < 0:
            continue
        d = (binds[i].translation - binds[p].translation).length
        if d > default * 0.1:
            lengths[p] = d if lengths[p] == default else min(lengths[p], d)
    return [min(max(l, default * 0.1), default * 4.0) for l in lengths]


def _joint_marker(name):
    mesh = bpy.data.meshes.new(name)
    verts = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]
    edges = [(i, j) for i in range(6) for j in range(i + 1, 6) if i // 2 != j // 2]
    mesh.from_pydata(verts, edges, [])
    # Referenced by the bones only; this isn't a scene mesh to render or export.
    return bpy.data.objects.new(name, mesh)


def _create_armature(context, collection, name, m, scale, align_bones=False):
    nodes = m.nodes
    parents = m.node_parents()
    nonterminal = {p for p in parents if p >= 0}
    binds = [convert.matrix_to_blender(n.bind_matrix, scale) for n in nodes]

    arm = bpy.data.armatures.new(name)
    arm.display_type = 'STICK'
    obj = bpy.data.objects.new(name, arm)
    collection.objects.link(obj)

    radius = m.internal_radius * scale or max((b.translation.length for b in binds), default=1.0)
    lengths = _bone_lengths(binds, parents, max(radius * 0.05, 1e-3))

    view_layer = context.view_layer
    prev_active = view_layer.objects.active
    view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    ebones = []
    for i, n in enumerate(nodes):
        eb = arm.edit_bones.new(n.name)
        eb.head = (0.0, 0.0, 0.0)
        eb.tail = (0.0, lengths[i], 0.0)
        eb.matrix = binds[i]
        if align_bones:
            children = [j for j, p in enumerate(parents) if p == i
                        and (binds[j].translation - eb.head).length > 1e-6]
            if children:
                child = min(children, key=lambda j: (binds[j].translation - eb.head).length)
                eb.tail = binds[child].translation
            elif parents[i] >= 0:
                parent = ebones[parents[i]]
                eb.tail = eb.head + (parent.tail - parent.head).normalized() * lengths[i]
            if i not in nonterminal:
                # Edit Mode still needs a nonzero bone, but not a long invented tail.
                reference = ebones[parents[i]].length if parents[i] >= 0 else lengths[i]
                eb.length = max(min(lengths[i], reference * 0.15), 1e-4 * scale)
        if parents[i] >= 0:
            eb.parent = ebones[parents[i]]
        ebones.append(eb)
    names = [eb.name for eb in ebones]
    bpy.ops.object.mode_set(mode='OBJECT')
    view_layer.objects.active = prev_active

    marker = None
    for i, n in enumerate(nodes):
        b = arm.bones[names[i]]
        b["lt_index"] = i
        b["lt_flags"] = n.flags
        if align_bones:
            correction = b.matrix_local.inverted() @ binds[i]
            b["lt_axis_correction"] = [correction[r][c] for r in range(4) for c in range(4)]
            if i not in nonterminal:
                if marker is None:
                    marker = _joint_marker(name + " joint marker")
                pb = obj.pose.bones[names[i]]
                pb.custom_shape = marker
                pb.use_custom_shape_bone_size = False
                pb.custom_shape_scale_xyz = (b.length * 0.5,) * 3
        if names[i] != n.name:
            b["lt_name"] = n.name
    return obj, names


def _material_for(name, texture_index, image):
    mat_name = "%s_skin%d" % (name, texture_index)
    mat = bpy.data.materials.get(mat_name)
    if mat is None:
        mat = textures.make_material(mat_name, image)
        mat["lt_texture_index"] = texture_index
    return mat


def group_weights(weights):
    """{node: vertex group weight} for a vertex's (node, bias) pairs: a
    node's biases summed and held to 0..1, as a vertex group can hold."""
    out = {}
    for node, bias in weights:
        out[node] = out.get(node, 0.0) + bias
    return {n: min(max(b, 0.0), 1.0) for n, b in out.items()}


def _create_mesh_object(bind_inv, name, lod, scale, bone_names):
    me = bpy.data.meshes.new(name)
    verts = [convert.vec_to_blender(v.location, scale) for v in lod.vertices]
    # The swap of axes turns the winding round: corners go in as 0, 2, 1.
    faces = [(t.indices[0], t.indices[2], t.indices[1]) for t in lod.triangles]
    me.from_pydata(verts, [], faces)

    uv = me.uv_layers.new(name="UVMap")
    flat = []
    for t in lod.triangles:
        for c in (0, 2, 1):
            u, v = t.uvs[c]
            flat += (u, 1.0 - v)
    uv.data.foreach_set("uv", flat)

    rep = me.attributes.new("lt_replacement", 'INT', 'POINT')
    rep.data.foreach_set("value", [v.replacement for v in lod.vertices])

    # The stored normals exactly (Blender keeps custom normals to about 1e-3,
    # can't keep a zero one and bends some, as on faces that share their
    # vertices back to back).
    nattr = me.attributes.new("lt_normal", 'FLOAT_VECTOR', 'POINT')
    nattr.data.foreach_set("vector", [c for v in lod.vertices for c in v.normal])

    # Vertices whose weights don't give back their position (some LODs, and
    # biases that don't sum to 1) keep their weights' stored points, for
    # the exporter to reuse while the vertex stays where it was.
    # (Only the sum of a vertex's points is fixed by its position, so each
    # point is checked against the one the exporter would write.)
    odd = []
    for vi, v in enumerate(lod.vertices):
        pv = Vector(v.location)
        nodes_v = [w.node for w in v.weights]
        if len(set(nodes_v)) != len(nodes_v) or \
                any(not (1e-6 < w.bias <= 1.0) for w in v.weights):
            # Vertex groups hold one weight per node, from 0 to 1.
            odd.append(vi)
            continue
        for w in v.weights:
            local = bind_inv[w.node] @ pv
            if max(abs(local[i] * w.bias - w.location[i]) for i in range(3)) > 1e-4:
                odd.append(vi)
                break
    if odd:
        slots = max(len(lod.vertices[vi].weights) for vi in odd)
        loc = me.attributes.new("lt_location", 'FLOAT_VECTOR', 'POINT')
        flat = [0.0] * (3 * len(lod.vertices))
        for vi in odd:
            flat[3 * vi:3 * vi + 3] = lod.vertices[vi].location
        loc.data.foreach_set("vector", flat)
        for k in range(slots):
            nodes_k = [-1] * len(lod.vertices)
            pts_k = [0.0] * (3 * len(lod.vertices))
            bias_k = [0.0] * len(lod.vertices)
            for vi in odd:
                ws = lod.vertices[vi].weights
                if k < len(ws):
                    nodes_k[vi] = ws[k].node
                    pts_k[3 * vi:3 * vi + 3] = ws[k].location
                    bias_k[vi] = ws[k].bias
            a = me.attributes.new("lt_weight%d_node" % k, 'INT', 'POINT')
            a.data.foreach_set("value", nodes_k)
            a = me.attributes.new("lt_weight%d_point" % k, 'FLOAT_VECTOR', 'POINT')
            a.data.foreach_set("vector", pts_k)
            a = me.attributes.new("lt_weight%d_bias" % k, 'FLOAT', 'POINT')
            a.data.foreach_set("value", bias_k)

    me.shade_smooth()
    normals = [convert.normal_to_blender(v.normal) for v in lod.vertices]
    me.normals_split_custom_set_from_vertices([n.normalized() if n.length else n for n in normals])
    me.update()
    # The corner normals Blender made of them, so the exporter can tell
    # which corners were edited since (it writes lt_normal for the rest).
    cn = [0.0] * (3 * len(me.loops))
    me.corner_normals.foreach_get("vector", cn)
    me.attributes.new("lt_import_normal", 'FLOAT_VECTOR', 'CORNER').data.foreach_set("vector", cn)

    obj = bpy.data.objects.new(name, me)
    groups = [obj.vertex_groups.new(name=bn) for bn in bone_names]
    batches = {}
    for vi, v in enumerate(lod.vertices):
        for node, weight in group_weights((w.node, w.bias) for w in v.weights).items():
            batches.setdefault((node, weight), []).append(vi)
    for (node, bias), vis in batches.items():
        groups[node].add(vis, bias, 'REPLACE')
    return obj


def _create_socket(collection, arm_obj, bone_names, s, index, scale):
    bone = arm_obj.data.bones[bone_names[s.node]]
    obj = bpy.data.objects.new(s.name, None)
    obj.empty_display_type = 'ARROWS'
    obj.empty_display_size = max(bone.length * 0.5, 1e-3)
    collection.objects.link(obj)
    obj.parent = arm_obj
    obj.parent_type = 'BONE'
    obj.parent_bone = bone.name
    # A bone child hangs off the bone's tail.
    obj.matrix_parent_inverse = Matrix.Identity(4)
    obj.matrix_basis = (Matrix.Translation((0.0, -bone.length, 0.0)) @
                        convert.bone_correction(bone) @
                        convert.transform_to_blender(s.position, s.rotation, scale))
    obj["lt_socket"] = True
    obj["lt_index"] = index
    if obj.name != s.name:
        obj["lt_name"] = s.name
    return obj


def _rest_locals(arm_obj, bone_names, parents):
    """Each bone's rest matrix relative to its parent's, in Blender space."""
    bones = arm_obj.data.bones
    out = []
    for i, bn in enumerate(bone_names):
        ml = bones[bn].matrix_local
        if parents[i] >= 0:
            ml = bones[bone_names[parents[i]]].matrix_local.inverted() @ ml
        out.append(ml.inverted())
    return out


def _create_action(arm_obj, action_name, anim, node_map, rest_inv_locals, scale, fps):
    """node_map: (bone name, index into anim.node_transforms) per bone."""
    action = bpy.data.actions.new(action_name)
    action.use_fake_user = True
    ad = arm_obj.animation_data or arm_obj.animation_data_create()
    ad.action = action

    frames = [k.time * fps / 1000.0 for k in anim.keyframes]
    strings = []
    for k, f in zip(anim.keyframes, frames):
        if k.command:
            mk = action.pose_markers.new(k.command)
            mk.frame = int(round(f))
            strings.append([k.time, k.command])
    # Marker names are cut at 63 bytes and sit on whole frames; the strings
    # as they were, for the exporter to use while the markers are unchanged.
    if strings:
        action["lt_key_strings_json"] = json.dumps(strings)
        action["lt_key_strings_fps"] = fps

    for bi, (bone_name, ti) in enumerate(node_map):
        bone = arm_obj.data.bones[bone_name]
        parent_axes = convert.bone_correction(bone.parent) if bone.parent else Matrix.Identity(4)
        axes_inv = convert.bone_correction(bone).inverted()
        if ti is None:
            # A node the animation's model doesn't have stays at rest (one
            # key, so it doesn't keep the pose of the action before).
            key_frames = frames[:1]
            locs, rots = [(0.0, 0.0, 0.0)], [(1.0, 0.0, 0.0, 0.0)]
        else:
            key_frames = frames
            locs, rots = [], []
            prev = None
            for pos, rot in anim.node_transforms[ti]:
                basis = (rest_inv_locals[bi] @ parent_axes @
                         convert.transform_to_blender(pos, rot, scale) @ axes_inv)
                loc, q, _ = basis.decompose()
                if prev is not None and prev.dot(q) < 0.0:
                    q.negate()
                prev = q
                locs.append(loc)
                rots.append(q)
        group = bone_name
        base = 'pose.bones["%s"].' % bpy.utils.escape_identifier(bone_name)
        for path, values, n in (("location", locs, 3), ("rotation_quaternion", rots, 4)):
            for c in range(n):
                fc = action.fcurve_ensure_for_datablock(arm_obj, base + path, index=c,
                                                        group_name=group)
                fc.keyframe_points.add(len(key_frames))
                co = []
                for f, v in zip(key_frames, values):
                    co += (f, v[c])
                fc.keyframe_points.foreach_set("co", co)
                fc.keyframe_points.foreach_set("interpolation", [1] * len(key_frames))  # LINEAR
                fc.update()
    return action


def load(context, filepath, *, scale=1.0, face_front=True, import_animations=True,
         child_animations=True, import_lods=True, import_sockets=True,
         import_textures=True, texture_dirs=()):
    m = abc.read_file(filepath)
    name = os.path.splitext(os.path.basename(filepath))[0]
    fps = context.scene.render.fps / context.scene.render.fps_base

    if context.object and context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    collection = bpy.data.collections.new(name)
    context.scene.collection.children.link(collection)

    arm_obj, bone_names = _create_armature(context, collection, name, m, scale, align_bones=True)
    nodes = m.nodes
    parents = m.node_parents()

    if face_front:
        # LithTech models face +Z, which is Blender's +Y; turn the armature
        # object to face -Y as Blender's front view expects. The exporter
        # works in the armature's own space, so this doesn't change the file.
        arm_obj.rotation_euler = (0.0, 0.0, math.pi)
    arm_obj["lt_version"] = m.version
    arm_obj["lt_command_string"] = m.command_string
    arm_obj["lt_internal_radius"] = m.internal_radius
    if m.lod_distances:
        arm_obj["lt_lod_distances"] = list(m.lod_distances)
    arm_obj["lt_scale"] = scale
    if any(m.header_reserved):
        arm_obj["lt_header_reserved"] = list(m.header_reserved)
    arm_obj["lt_weight_sets_json"] = json.dumps(
        [{"name": ws.name, "weights": {nodes[i].name: w for i, w in enumerate(ws.weights)}}
         for ws in m.weight_sets])
    ident = ((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0))

    def relation(c):
        return {nodes[i].name: [list(p), list(q)] for i, (p, q) in enumerate(c.relation)
                if (tuple(p), tuple(q)) != ident}

    if m.child_models:
        arm_obj["lt_save_index"] = m.child_models[0].save_index
        arm_obj["lt_relation_json"] = json.dumps(relation(m.child_models[0]))
    arm_obj["lt_child_models_json"] = json.dumps(
        [{"name": c.name, "save_index": c.save_index,
          "relation": relation(c),
          "anim_bindings": [[b.name, list(b.dims), list(b.translation)] for b in bindings]}
         for c, bindings in zip(m.child_models, m.anim_bindings)][1:])

    # Pieces and their LODs.
    bind_inv = [Matrix((n.bind_matrix[0:4], n.bind_matrix[4:8], n.bind_matrix[8:12],
                        n.bind_matrix[12:16])).inverted_safe() for n in nodes]
    model_dir = os.path.dirname(filepath)
    images = {}
    lod_collection = None
    for pi, p in enumerate(m.pieces):
        image = None
        if import_textures:
            if p.texture_index not in images:
                images[p.texture_index] = textures.find_skin(
                    filepath, p.texture_index, len({q.texture_index for q in m.pieces}),
                    texture_dirs)
            image = images[p.texture_index]
        mat = _material_for(name, p.texture_index, image)
        for li, lod in enumerate(p.lods):
            if li > 0 and not import_lods:
                break
            obj_name = p.name if li == 0 else "%s_LOD%d" % (p.name, li)
            obj = _create_mesh_object(bind_inv, obj_name, lod, scale, bone_names)
            obj.data.materials.append(mat)
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
            if li == 0:
                obj["lt_texture_index"] = p.texture_index
                obj["lt_specular_power"] = p.specular_power
                obj["lt_specular_scale"] = p.specular_scale
                obj["lt_lod_weight"] = p.lod_weight
                if p.reserved:
                    obj["lt_reserved"] = list(p.reserved)

    if import_sockets:
        for si, s in enumerate(m.sockets):
            _create_socket(collection, arm_obj, bone_names, s, si, scale)

    if import_animations:
        rest_inv = _rest_locals(arm_obj, bone_names, parents)
        own_map = [(bn, i) for i, bn in enumerate(bone_names)]
        first = None
        for ai, a in enumerate(m.animations):
            act = _create_action(arm_obj, a.name, a, own_map, rest_inv, scale, fps)
            act["lt_model"] = name
            act["lt_index"] = ai
            act["lt_dims"] = list(a.dims)
            act["lt_translation"] = list(m.anim_bindings[0][ai].translation) \
                if m.anim_bindings and ai < len(m.anim_bindings[0]) else [0.0, 0.0, 0.0]
            act["lt_interpolation_ms"] = a.interpolation_time
            act["lt_weight_set"] = a.modeledit_weight_set
            first = first or act

        if child_animations:
            for ci, c in enumerate(m.child_models[1:], 1):
                path = _find_file_nocase(model_dir, c.name)
                if not path:
                    print("LithTech: child model %r not found next to %s" % (c.name, filepath))
                    continue
                try:
                    cm = abc.read_file(path)
                except abc.ABCError as e:
                    print("LithTech: can't read child model %s: %s" % (path, e))
                    continue
                cidx = {n.name.lower(): i for i, n in enumerate(cm.nodes)}
                cmap = [(bn, cidx.get(nodes[i].name.lower())) for i, bn in enumerate(bone_names)]
                for ai, a in enumerate(cm.animations):
                    act = _create_action(arm_obj, a.name, a, cmap, rest_inv, scale, fps)
                    act["lt_model"] = name
                    act["lt_child"] = c.name
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
