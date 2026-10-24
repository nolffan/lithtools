"""The PS2 exporter's rebuilding path: each model is imported, its meshes
lose what the import kept (the stored strips and weights), and the export
must rebuild them: the same triangles (model-space corners and UVs), the
same weights to within their 1/4096 steps, and a chain that reads back.
Also subdivides one mesh of each model and checks the result reads, with
four times the triangles.

    blender -b --factory-startup --python tests/blender_rebuild_ps2ltb.py -- \\
        [--every K] [paths or directories...]
"""

import os
import sys
import tempfile
import time
import traceback
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

import bpy  # noqa: E402
import io_scene_lithtech  # noqa: E402
from io_scene_lithtech import export_ps2ltb, import_ps2ltb  # noqa: E402
from io_scene_lithtech.lithtech import ps2ltb  # noqa: E402

ASSETS = HERE.parents[2] / "out/assets/ps2"


def purge():
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.armatures, bpy.data.actions,
                 bpy.data.materials, bpy.data.images, bpy.data.collections):
        try:
            bpy.data.batch_remove(list(coll))
        except Exception:
            pass


def apply(m16, v):
    return tuple(m16[4 * r] * v[0] + m16[4 * r + 1] * v[1] + m16[4 * r + 2] * v[2] + m16[4 * r + 3] for r in range(3))


def triangles(m, lod):
    """{corner positions (rounded, rotated to start at the smallest, the
    winding kept): [their UVs per copy]}."""
    out = {}
    for s in lod.strips():
        for tri in s.triangles():
            cs = []
            for c in tri:
                v = s.vertices[c]
                p = v.position[:3] if lod.kind == ps2ltb.SKEL_MESH else apply(m.nodes[lod.node].bind_matrix, v.position)
                cs.append((tuple(round(x, 2) for x in p), v.stq[:2]))
            k = cs.index(min(cs))
            cs = cs[k:] + cs[:k]
            if len({c[0] for c in cs}) == 3:
                out.setdefault(tuple(c[0] for c in cs), []).append([c[1] for c in cs])
    return out


def same_triangles(ta, tb):
    """How many triangles differ (positions, or UVs beyond 1e-4)."""
    bad = 0
    for key in set(ta) | set(tb):
        ua, ub = ta.get(key, []), list(tb.get(key, []))
        for uv in ua:
            hit = next((i for i, w in enumerate(ub) if all(abs(x - y) <= 1e-4 * max(1.0, abs(x))
                                                            for c, d in zip(uv, w) for x, y in zip(c, d))), None)
            if hit is None:
                bad += 1
            else:
                ub.pop(hit)
        bad += len(ub)
    return bad


def weights(m, lod):
    if lod.kind != ps2ltb.SKEL_MESH:
        return {}
    out = {}
    for u in lod.uniques:
        key = tuple(round(x, 2) for x in u.position[:3])
        out[key] = import_ps2ltb.group_weights(u.node_weights(lod.bone_map))
    return out


def main(argv):
    every, paths = 1, []
    i = 0
    while i < len(argv):
        if argv[i] == "--every":
            every = int(argv[i + 1])
            i += 2
        else:
            paths.append(Path(argv[i]))
            i += 1
    files = []
    for p in paths or [ASSETS]:
        files += sorted(f for f in p.rglob("*") if f.suffix.lower() == ".ltb") if p.is_dir() else [p]
    files = [f for f in files if ps2ltb.is_ps2_model(f.read_bytes()[:8])][::every]
    bad = 0
    t0 = time.time()
    tmp = tempfile.mkdtemp()
    stats = Counter()
    for f in files:
        if os.environ.get("PS2_TRACE"):
            print("model", f, flush=True)
        purge()
        err = []
        try:
            a = ps2ltb.read_file(str(f))
            arm = import_ps2ltb.load(bpy.context, str(f), import_textures=False, child_animations=False,
                                     import_animations=False)
            meshes = [o for o in arm.children if o.type == 'MESH']
            for o in meshes:
                for k in ("lt_ps2_strips", "lt_ps2_bone_map"):
                    if k in o:
                        del o[k]
                for attr in ("lt_ps2_weights", "lt_ps2_bones"):
                    if attr in o.data.attributes:
                        o.data.attributes.remove(o.data.attributes[attr])
            out = os.path.join(tmp, "out.ltb")
            export_ps2ltb.save(bpy.context, out, armature=arm, export_animations=False)
            b = ps2ltb.read_file(out)
            for pi, (p, q) in enumerate(zip(a.pieces, b.pieces)):
                for li, (l, k) in enumerate(zip(p.lods, q.lods)):
                    if l.kind != k.kind:
                        err.append("piece %d lod %d kind %d -> %d" % (pi, li, l.kind, k.kind))
                    nbad = same_triangles(triangles(a, l), triangles(b, k))
                    if nbad:
                        err.append("piece %d lod %d: %d triangles differ" % (pi, li, nbad))
                    wa, wb = weights(a, l), weights(b, k)
                    for key, w in wa.items():
                        v = wb.get(key)
                        if v is None or set(v) != set(w) or any(abs(v[n] - w[n]) > 2.5 / 4096 for n in w):
                            err.append("piece %d lod %d weights" % (pi, li))
                            break
                    stats["strip vertices before"] += sum(len(s.vertices) for s in l.strips())
                    stats["strip vertices after"] += sum(len(s.vertices) for s in k.strips())
                    stats["buffers over 844"] += sum(1 for bf in k.buffers if 1 + sum(1 + 3 * len(s.vertices) for s in bf.strips) > 844)
            # new geometry: subdivide the first mesh
            if meshes:
                o = meshes[0]
                n0 = len(o.data.polygons)
                bpy.context.view_layer.objects.active = o
                bpy.ops.object.mode_set(mode='EDIT')
                bpy.ops.mesh.select_all(action='SELECT')
                bpy.ops.mesh.subdivide()
                bpy.ops.object.mode_set(mode='OBJECT')
                export_ps2ltb.save(bpy.context, out, armature=arm, export_animations=False)
                c = ps2ltb.read_file(out)
                pi = int(o.get("lt_piece_index", 0))
                n1 = c.pieces[pi].lods[0].triangle_count()
                if n1 != 4 * n0:
                    err.append("subdivided: %d triangles, want %d" % (n1, 4 * n0))
        except Exception:
            err = [traceback.format_exc()]
        if err:
            bad += 1
            print("FAIL %s: %s" % (f, "; ".join(err[:4])))
    print("%d models, %d failed, %.0fs; %s" % (len(files), bad, time.time() - t0, dict(stats)))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
