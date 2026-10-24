"""Import a PS2 NOLF model (.ltb, file type 2, version 16).

The same layout as import_abc and import_ltb (armature, a mesh per piece,
hidden LOD objects, socket empties, actions), with what the PS2 adds:

  * the triangle strips become faces. Rigid vertices are told apart by
    position and normal (as the PC models' are) and moved from their node's
    space into model space; skeletal meshes use their unique vertices
  * each strip vertex's s, t go into the UV map (per corner)
  * per LOD: the strips as they were (lt_ps2_strips: buffers of strips of
    [vertex, s, t, extra]), so an unchanged mesh exports with the same
    strips; the raw positions and normals (lt_ps2_position, lt_ps2_normal,
    rigid ones in node space) and the stored weights (lt_ps2_weights,
    lt_ps2_bones; lt_ps2_bone_map per object)
  * names the file keeps only as hashes come from lithtech/ps2ltb_names.txt;
    one with no name is "#xxxxxxxx" and exports as that hash
"""

import base64
import json
import math
import os
import struct

import bpy

from .lithtech import abc, ps2ltb
from . import butes
from . import convert
from . import import_abc
from . import textures


class _Skeleton:
    def __init__(self, m):
        self.nodes = [abc.Node(n.name, i, n.flags, tuple(n.bind_matrix)) for i, n in enumerate(m.nodes)]
        self._parents = m.node_parents()
        self.internal_radius = m.radius

    def node_parents(self):
        return self._parents


class _Keyed:
    """A PS2 animation as import_abc._create_action reads one."""

    def __init__(self, anim, nnodes):
        self.keyframes = anim.keyframes
        self.node_transforms = [[anim.key_transform(n, k) for k in range(len(anim.keyframes))]
                                for n in range(nnodes)]


class _Socket:
    def __init__(self, node, name, position, rotation):
        self.node, self.name, self.position, self.rotation = node, name, position, rotation


def _mat_apply(m, v):
    return tuple(m[4 * r] * v[0] + m[4 * r + 1] * v[1] + m[4 * r + 2] * v[2] + m[4 * r + 3] for r in range(3))


def _mat_rotate(m, v):
    return tuple(m[4 * r] * v[0] + m[4 * r + 1] * v[1] + m[4 * r + 2] * v[2] for r in range(3))


def mesh_from_lod(m, lod):
    """(vertices [(model position, model normal, raw position, raw normal)],
    faces [((vertex, (s, t)) x3) in PC order], strips json, weights per
    vertex [(node, w)], raw weights per vertex) for a LOD."""
    verts, raw_w = [], []
    index = {}
    if lod.kind == ps2ltb.SKEL_MESH:
        for u in lod.uniques:
            verts.append((u.position[:3], u.normal[:3], u.position, u.normal))
            raw_w.append((u.weights, u.bones))

        def vid(v):
            return v.unique
    else:
        bind = m.nodes[lod.node].bind_matrix if lod.node < len(m.nodes) else (1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0)

        def vid(v):
            k = (v.position, v.normal)
            i = index.get(k)
            if i is None:
                i = index[k] = len(verts)
                verts.append((_mat_apply(bind, v.position), _mat_rotate(bind, v.normal), v.position, v.normal))
            return i
    faces, buffers = [], []
    for b in lod.buffers:
        sj = []
        for s in b.strips:
            ids = [vid(v) for v in s.vertices]
            for t in s.triangles():
                faces.append(tuple((ids[c], (s.vertices[c].stq[0], s.vertices[c].stq[1])) for c in t))
            sj.append([int(s.reversed), s.gif_hi, [[i, v.stq[0], v.stq[1], v.extra] for i, v in zip(ids, s.vertices)]])
        buffers.append(sj)
    if lod.kind == ps2ltb.SKEL_MESH:
        weights = [u.node_weights(lod.bone_map) for u in lod.uniques]
    else:
        weights = [[(lod.node, 1.0)] for _ in verts]
    return verts, faces, buffers, weights, raw_w


def group_weights(weights):
    """{node: weight} as vertex groups hold [(node, w)]: a node in two slots
    summed, then clamped to 0..1 (some files store weights outside it)."""
    out = {}
    for node, w in weights:
        out[node] = out.get(node, 0.0) + w
    return {n: max(0.0, min(1.0, w)) for n, w in out.items() if w > 0.0}


def _create_lod_object(name, m, lod, scale, bone_names):
    verts, faces, buffers, weights, raw_w = mesh_from_lod(m, lod)
    me = bpy.data.meshes.new(name)
    good, bad = [], []
    # A face on the same three vertices as an earlier one (double-sided
    # meshes) gets copies of them: Blender's edit tools can't take two faces
    # on one set of vertices. `source` gives each Blender vertex's vertex in
    # the file (lt_ps2_unique), which the strips refer to.
    source = list(range(len(verts)))
    copies = {}
    seen = set()
    for fi, f in enumerate(faces):
        ids = (f[0][0], f[2][0], f[1][0])   # the axis swap turns the winding round
        if len(set(ids)) != 3:
            bad.append((fi, ids, f))
            continue
        level = 0
        while (frozenset(ids), level) in seen:
            level += 1
        seen.add((frozenset(ids), level))
        if level:
            new = []
            for v in ids:
                k = (v, level)
                if k not in copies:
                    copies[k] = len(source)
                    source.append(v)
                new.append(copies[k])
            ids = tuple(new)
        good.append((fi, ids, f))
    verts = [verts[i] for i in source]
    weights = [weights[i] for i in source]
    raw_w = [raw_w[i] for i in source] if raw_w else raw_w
    me.from_pydata([convert.vec_to_blender(v[0], scale) for v in verts], [], [g[1] for g in good])
    me.attributes.new("lt_ps2_unique", 'INT', 'POINT').data.foreach_set("value", source)
    uv = me.uv_layers.new(name="UVMap")
    flat = []
    for _, _, f in good:
        for c in (f[0], f[2], f[1]):
            s, t = c[1]
            flat += (s, 1.0 - t)
    uv.data.foreach_set("uv", flat)
    me.shade_smooth()
    for attr, k in (("lt_ps2_position", 2), ("lt_ps2_normal", 3)):
        a = me.attributes.new(attr, 'FLOAT_COLOR', 'POINT')   # four floats a vertex, as stored
        a.data.foreach_set("color", [c for v in verts for c in v[k]])
    me.normals_split_custom_set_from_vertices([convert.normal_to_blender(v[1]).normalized() for v in verts])
    me.update()
    # the normals Blender made of them (per vertex, its corners' sum, as the
    # exporter reads them): the raw ones export while these stay the same
    sums = [[0.0, 0.0, 0.0] for _ in verts]
    cn = [0.0] * (3 * len(me.loops))
    me.corner_normals.foreach_get("vector", cn)
    for li, loop in enumerate(me.loops):
        sv = sums[loop.vertex_index]
        sv[0] += cn[3 * li]
        sv[1] += cn[3 * li + 1]
        sv[2] += cn[3 * li + 2]
    me.attributes.new("lt_ps2_import_normal", 'FLOAT_VECTOR', 'POINT').data.foreach_set(
        "vector", [c for sv in sums for c in sv])
    if raw_w:
        a = me.attributes.new("lt_ps2_weights", 'FLOAT_COLOR', 'POINT')
        a.data.foreach_set("color", [float(x) for w, _ in raw_w for x in w])
        a = me.attributes.new("lt_ps2_bones", 'INT', 'POINT')
        a.data.foreach_set("value", [int.from_bytes(bytes(b), "little", signed=True) for _, b in raw_w])

    obj = bpy.data.objects.new(name, me)
    groups = [obj.vertex_groups.new(name=bn) for bn in bone_names]
    batches = {}
    for vi, ws in enumerate(weights):
        for node, w in group_weights(ws).items():
            if 0 <= node < len(groups):
                batches.setdefault((node, w), []).append(vi)
    for (node, w), vis in batches.items():
        groups[node].add(vis, w, 'REPLACE')

    obj["lt_mesh_kind"] = lod.kind
    if lod.kind == ps2ltb.RIGID_MESH:
        obj["lt_bone_effector"] = bone_names[lod.node] if lod.node < len(bone_names) else ""
    else:
        obj["lt_ps2_bone_map"] = list(lod.bone_map)
    obj["lt_ps2_strips"] = json.dumps(buffers, separators=(",", ":"))
    obj["lt_ps2_faces"] = len(faces)
    if bad:
        obj["lt_degenerate_faces"] = json.dumps([[fi] + list(ids) for fi, ids, _ in bad])
    return obj


def load(context, filepath, *, scale=1.0, face_front=True, import_animations=True,
         child_animations=True, import_lods=True, import_sockets=True,
         import_textures=True, texture_dirs=()):
    m = ps2ltb.read_file(filepath)
    name = os.path.splitext(os.path.basename(filepath))[0]
    fps = context.scene.render.fps / context.scene.render.fps_base
    if context.object and context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    collection = bpy.data.collections.new(name)
    context.scene.collection.children.link(collection)

    skel = _Skeleton(m)
    arm_obj, bone_names = import_abc._create_armature(context, collection, name, skel, scale)
    parents = skel.node_parents()
    if face_front:
        arm_obj.rotation_euler = (0.0, 0.0, math.pi)
    arm_obj["lt_format"] = "ps2ltb"
    arm_obj["lt_command_string"] = m.command
    arm_obj["lt_internal_radius"] = m.radius
    arm_obj["lt_scale"] = scale
    arm_obj["lt_hash_mult"] = m.hash_mult
    arm_obj["lt_ps2_unknown"] = m.unknown
    arm_obj["lt_lod_distances"] = list(m.lod_distances)
    arm_obj["lt_ps2_header_rest"] = m.header_rest.hex()
    # the stored counts, and what they'd be computed from the file as read
    # (the exporter keeps the stored ones while the model computes the same)
    arm_obj["lt_ps2_counts"] = list(m.counts)
    arm_obj["lt_ps2_counts_computed"] = m.computed_counts()
    nodes = m.nodes
    arm_obj["lt_weight_sets_json"] = json.dumps(
        [{"name": m.weight_set_name(ws), "weights": {nodes[i].name: w for i, w in enumerate(ws.weights) if i < len(nodes)}}
         for ws in m.weight_sets])
    arm_obj["lt_child_models_json"] = json.dumps(m.child_models if m.child_models is not None else None)
    for i, n in enumerate(nodes):
        b = arm_obj.data.bones[bone_names[i]]
        # as stored, for export while the bone stays where it was imported
        # (a bone holds it only to float32 precision)
        b["lt_bind"] = list(n.bind_matrix)

    explicit = ()
    if import_textures:
        found = butes.model_skins(filepath)
        if found:
            arm_obj["lt_butes_model"] = found[0]
            explicit = found[1]

    images = {}
    lod_collection = None
    slot_count = len({p.texture for p in m.pieces})
    for pi, p in enumerate(m.pieces):
        pname = m.piece_name(p)
        for li, lod in enumerate(p.lods):
            if li > 0 and not import_lods:
                break
            obj_name = pname if li == 0 else "%s_LOD%d" % (pname, li)
            obj = _create_lod_object(obj_name, m, lod, scale, bone_names)
            image = None
            if import_textures:
                if p.texture not in images:
                    try:
                        images[p.texture] = textures.find_skin(filepath, p.texture, slot_count, texture_dirs, explicit)
                    except Exception as e:   # a PS2 skin dtx.py can't decode yet
                        print("LithTech: skin %d of %s: %s" % (p.texture, name, e))
                        images[p.texture] = None
                image = images[p.texture]
            obj.data.materials.append(import_abc._material_for(name, p.texture, image))
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
            obj["lt_piece"] = pname
            obj["lt_piece_index"] = pi
            obj["lt_lod"] = li
            obj["lt_texture_index"] = p.texture
            if li == 0:
                obj["lt_lod_count"] = len(p.lods)
                obj["lt_ps2_colors"] = list(p.colors)
                obj["lt_ps2_piece_unknown"] = list(p.unknown)

    if import_sockets:
        for si, s in enumerate(m.sockets):
            sname = m.socket_name(s)
            e = import_abc._create_socket(collection, arm_obj, bone_names,
                                          _Socket(s.node, sname, s.position, s.rotation), si, scale)
            # the hashes as stored, for the name they were imported under
            e["lt_ps2_socket_hashes"] = "%08x %08x" % (s.name_hash, s.name_hash1)
            e["lt_ps2_socket_name"] = sname
            # the transform as stored, while the empty stays where it was put
            e["lt_ps2_socket_raw"] = list(s.rotation) + list(s.position)
            e["lt_ps2_socket_basis"] = [c for row in e.matrix_basis for c in row]
            if abs(sum(c * c for c in s.rotation) - 1.0) > 1e-4:
                e["lt_socket_rotation"] = list(s.rotation)
                e["lt_socket_basis_rotation"] = list(e.matrix_basis.to_quaternion())

    if import_animations:
        rest_inv = import_abc._rest_locals(arm_obj, bone_names, parents)
        own_map = [(bn, i) for i, bn in enumerate(bone_names)]
        first = None
        for ai, a in enumerate(m.animations):
            aname = m.animation_name(a)
            act = import_abc._create_action(arm_obj, aname, _Keyed(a, len(nodes)), own_map, rest_inv, scale, fps)
            act["lt_model"] = name
            act["lt_index"] = ai
            act["lt_name"] = aname
            act["lt_dims"] = list(a.dims)
            act["lt_translation"] = list(a.translation)
            act["lt_interpolation_ms"] = a.interpolation_time
            # which nodes' positions use the coarse scale (1/16)
            act["lt_ps2_coarse"] = "".join("1" if any(v[3] for v in keys) else "0" for _, keys in a.tracks)
            # the keys as stored (8 s16 each, node by node): Blender keeps unit
            # quaternions only, so a stored one is a step off once it has been
            # through an F-curve; the exporter writes the stored key back while
            # the action's is within a step of it
            act["lt_ps2_tracks"] = base64.b64encode(b"".join(
                struct.pack("<8h", *v) for _, keys in a.tracks for v in keys)).decode("ascii")
            first = first or act
        if child_animations and m.child_models:
            model_dir = os.path.dirname(filepath)
            for c in m.child_models:
                path = import_abc._find_file_nocase(model_dir, c)
                if not path:
                    print("LithTech: child model %r not found next to %s" % (c, filepath))
                    continue
                try:
                    cm = ps2ltb.read_file(path)
                except Exception as e:
                    print("LithTech: can't read child model %s: %s" % (path, e))
                    continue
                cidx = {n.name.lower(): i for i, n in enumerate(cm.nodes)}
                cmap = [(bn, cidx.get(nodes[i].name.lower())) for i, bn in enumerate(bone_names)]
                for ai, a in enumerate(cm.animations):
                    act = import_abc._create_action(arm_obj, cm.animation_name(a), _Keyed(a, len(cm.nodes)),
                                                    cmap, rest_inv, scale, fps)
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
