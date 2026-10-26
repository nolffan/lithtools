"""Aligned bones, source poses, sockets, action slots and edited exports.

    blender -b --factory-startup --python-exit-code 1 --python plugins/blender/tests/blender_rig_checks.py
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import bpy
from mathutils import Vector
from io_scene_lithtech import animation, convert, export_abc, import_abc
from io_scene_lithtech.lithtech import abc


def close(a, b, tolerance=1e-4):
    assert max(abs(a[r][c] - b[r][c]) for r in range(4) for c in range(4)) < tolerance


def main():
    root = abc.Node("root", 0, 0, abc.quat_to_matrix((0, 0, 0), (0, 0, 0, 1)))
    child = abc.Node("tip", 1, 0, abc.quat_to_matrix((2, 0, 0), (0, 0, 0, 1)))
    root.children.append(child)
    m = abc.Model(root=root, internal_radius=4)
    collection = bpy.data.collections.new("Rig checks")
    bpy.context.scene.collection.children.link(collection)
    arm, names = import_abc._create_armature(bpy.context, collection, "Rig checks", m, 1, align_bones=True)
    arm["lt_scale"] = 1.0
    bpy.context.view_layer.objects.active = arm
    for n, name in zip(m.nodes, names):
        close(convert.bone_matrix(arm.data.bones[name]), convert.matrix_to_blender(n.bind_matrix))
    close(arm.data.bones[names[0]].matrix_local @ convert.bone_correction(arm.data.bones[names[0]]),
          convert.matrix_to_blender(root.bind_matrix))
    assert (arm.data.bones[names[0]].tail_local - arm.data.bones[names[1]].head_local).length < 1e-6
    assert arm.pose.bones[names[1]].custom_shape is not None
    assert not arm.pose.bones[names[1]].custom_shape.users_collection

    rotation = (0, 0, math.sin(0.3), math.cos(0.3))
    anim = abc.Animation("move", (2, 2, 2), keyframes=[abc.Keyframe(0, ""), abc.Keyframe(1000, "")],
                         node_transforms=[[((0, 0, 0), (0, 0, 0, 1)), ((1, 0, 0), rotation)],
                                          [((2, 0, 0), (0, 0, 0, 1)), ((2, 0, 0), rotation)]])
    fps = bpy.context.scene.render.fps / bpy.context.scene.render.fps_base
    action = import_abc._create_action(arm, "move", anim, [(name, i) for i, name in enumerate(names)],
                                      import_abc._rest_locals(arm, names, m.node_parents()), 1, fps)
    action["lt_model"] = arm.name
    animation.select_action(bpy.context, arm, action)
    assert arm.animation_data.action_slot in list(action.slots)
    assert len(export_abc._action_fcurves(action, arm)) == 14
    for key, frame in enumerate((0.0, fps)):
        bpy.context.scene.frame_set(math.floor(frame), subframe=frame % 1.0)
        bpy.context.view_layer.update()
        for name, expected in zip(names, abc.pose_matrices(m, anim, key)):
            bone = arm.data.bones[name]
            close(arm.pose.bones[name].matrix @ convert.bone_correction(bone), convert.matrix_to_blender(expected))

    animation.select_action(bpy.context, arm, None)
    sock = abc.Socket(1, "attachment", (0, 0, 0, 1), (0, 0.5, 0))
    empty = import_abc._create_socket(collection, arm, names, sock, 0, 1)
    bpy.context.view_layer.update()
    expected = convert.matrix_to_blender(child.bind_matrix) @ convert.transform_to_blender(sock.position, sock.rotation)
    close(empty.matrix_world, expected)
    nodes, _, _ = export_abc._build_skeleton(arm, 1)
    for before, after in zip(m.nodes, nodes):
        close(convert.matrix_to_blender(before.bind_matrix), convert.matrix_to_blender(after.bind_matrix))

    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones[names[1]]
    offset = Vector((0, 0.25, 0))
    edit.head += offset
    edit.tail += offset
    bpy.ops.object.mode_set(mode='OBJECT')
    nodes, _, _ = export_abc._build_skeleton(arm, 1)
    exported = convert.matrix_to_blender(nodes[1].bind_matrix)
    target = convert.matrix_to_blender(child.bind_matrix).translation + offset
    assert (exported.translation - target).length < 1e-4
    print("Bone display, animation slots/poses, sockets and edited export checks passed")


if __name__ == "__main__":
    main()
