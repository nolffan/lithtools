"""Between LithTech's space and Blender's.

LithTech is left-handed with Y up (+X right, +Y up, +Z forward); Blender is
right-handed with Z up (+X right, +Y forward/away, +Z up). Swapping Y and Z
maps one onto the other: the swap is a reflection, so it turns the
handedness round as well as the up axis. A matrix M in LithTech space is
SWAP @ M @ SWAP in Blender's (SWAP is its own inverse).

`scale` is Blender units per LithTech unit; it applies to positions only.
"""

from mathutils import Matrix, Vector

SWAP = Matrix(((1.0, 0.0, 0.0, 0.0),
               (0.0, 0.0, 1.0, 0.0),
               (0.0, 1.0, 0.0, 0.0),
               (0.0, 0.0, 0.0, 1.0)))


def vec_to_blender(v, scale=1.0):
    return Vector((v[0] * scale, v[2] * scale, v[1] * scale))


def vec_from_blender(v, scale=1.0):
    return (v[0] * scale, v[2] * scale, v[1] * scale)


def normal_to_blender(n):
    return Vector((n[0], n[2], n[1]))


def normal_from_blender(n):
    return (n[0], n[2], n[1])


def matrix_to_blender(m16, scale=1.0):
    """A row-major 16-float LithTech matrix as a Blender Matrix."""
    m = SWAP @ Matrix((m16[0:4], m16[4:8], m16[8:12], m16[12:16])) @ SWAP
    m.translation = m.translation * scale
    return m


def matrix_from_blender(m, scale=1.0):
    r = SWAP @ m @ SWAP
    r.translation = r.translation * scale
    return tuple(r[i][j] for i in range(4) for j in range(4))


def bone_correction(bone):
    """From the source node's axes to the displayed Blender bone's axes."""
    values = bone.get("lt_axis_correction")
    return Matrix((values[0:4], values[4:8], values[8:12], values[12:16])) \
        if values is not None else Matrix.Identity(4)


def bone_matrix(bone):
    """The source node's rest transform, including edits to the Blender bone."""
    return bone.matrix_local @ bone_correction(bone)


def transform_to_blender(position, rotation, scale=1.0):
    """A LithTech (position, (x, y, z, w)) pair as a Blender Matrix."""
    from .lithtech import abc
    return matrix_to_blender(abc.quat_to_matrix(position, rotation), scale)


def transform_from_blender(m, scale=1.0):
    """A rigid Blender Matrix as a LithTech (position, (x, y, z, w))."""
    r = SWAP @ m @ SWAP
    q = r.to_quaternion()
    t = r.translation * scale
    return ((t[0], t[1], t[2]), (q.x, q.y, q.z, q.w))
