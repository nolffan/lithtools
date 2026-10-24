"""Imports each extracted .ltb into Blender, exports it again and compares
the two models field by field (floats within a tolerance, everything else
exactly). Chunked files, which the games can't load, are skipped.

    blender -b --factory-startup --python tests/blender_roundtrip_ltb.py -- \
        [--limit N] [--every K] [paths or directories...]

Defaults to every model under out/assets/ltb (fill it with
extract_ltb.py). Exits non-zero if any model fails.
"""

import math
import os
import sys
import tempfile
import time
import traceback
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

import bpy  # noqa: E402
import io_scene_lithtech  # noqa: E402
from io_scene_lithtech import export_ltb, import_ltb  # noqa: E402
from io_scene_lithtech.lithtech import ltb  # noqa: E402

ASSETS = HERE.parents[2] / "out/assets/ltb"

TOL_POS = 2e-3
TOL_DIR = 1e-4
TOL_UV = 1e-6
TOL_QUAT = 2e-5


def purge():
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.armatures, bpy.data.actions,
                 bpy.data.materials, bpy.data.images, bpy.data.collections):
        try:
            bpy.data.batch_remove(list(coll))
        except Exception:
            pass


def vdiff(a, b):
    return max((abs(x - y) for x, y in zip(a, b)), default=0.0) if len(a) == len(b) else float("inf")


def qdiff(a, b):
    la = math.sqrt(sum(c * c for c in a))
    lb = math.sqrt(sum(c * c for c in b))
    if la < 1e-9 and lb < 1e-9:
        return 0.0   # both the zero quaternion (stored as such)
    la, lb = la or 1.0, lb or 1.0
    return 1.0 - abs(sum(x * y for x, y in zip(a, b))) / (la * lb)


class Diff:
    def __init__(self):
        self.errors = []
        self.max = {}

    def eq(self, what, a, b):
        if a != b:
            s = "%s: %r != %r" % (what, a, b)
            self.errors.append(s if len(s) < 300 else s[:300] + "...")

    def num(self, kind, what, d, tol):
        if d > self.max.get(kind, (0.0,))[0]:
            self.max[kind] = (d, what)
        if d > tol or math.isnan(d):
            self.errors.append("%s: off by %g" % (what, d))


def compare(a, b):
    d = Diff()
    d.eq("version", a.version, b.version)
    d.eq("header", a.header, b.header)
    d.eq("command string", a.command_string, b.command_string)
    d.eq("radius", a.radius, b.radius)
    d.eq("obbs", a.obbs, b.obbs)
    for i in ltb.ALLOC_EXACT:
        if i < len(a.alloc_counts):
            d.eq("alloc " + ltb.ALLOC_NAMES[i], a.alloc_counts[i], b.alloc_counts[i])
    an, bn = a.nodes, b.nodes
    d.eq("node names", [n.name for n in an], [n.name for n in bn])
    d.eq("node flags", [n.flags for n in an], [n.flags for n in bn])
    d.eq("node parents", a.node_parents(), b.node_parents())
    for x, y in zip(an, bn):
        d.num("bind_rot", "bind %s" % x.name, max(abs(x.bind_matrix[i] - y.bind_matrix[i])
                                                  for i in (0, 1, 2, 4, 5, 6, 8, 9, 10)), TOL_DIR)
        d.num("bind_pos", "bind %s" % x.name, vdiff([x.bind_matrix[i] for i in (3, 7, 11)],
                                                    [y.bind_matrix[i] for i in (3, 7, 11)]), TOL_POS)
    d.eq("pieces", [p.name for p in a.pieces], [p.name for p in b.pieces])
    for p, q in zip(a.pieces, b.pieces):
        d.eq(p.name + " lods", len(p.lods), len(q.lods))
        d.eq(p.name + " lod distances", p.lod_distances, q.lod_distances)
        d.eq(p.name + " lod offsets", (p.min_lod_offset, p.max_lod_offset), (q.min_lod_offset, q.max_lod_offset))
        for li, (l1, l2) in enumerate(zip(p.lods, q.lods)):
            w = "%s lod%d" % (p.name, li)
            d.eq(w + " textures", (l1.texture_count, l1.textures, l1.render_style, l1.render_priority),
                 (l2.texture_count, l2.textures, l2.render_style, l2.render_priority))
            d.eq(w + " used nodes", l1.used_nodes, l2.used_nodes)
            m1, m2 = l1.mesh, l2.mesh
            d.eq(w + " kind", m1.kind, m2.kind)
            d.eq(w + " layout", (m1.stream_flags, m1.max_bones_per_tri, m1.max_bones_per_vert, m1.bone_effector,
                                 m1.reindexed_bones_flag, m1.use_matrix_palettes, m1.min_bone, m1.max_bone,
                                 m1.reindexed_bones, m1.undup_vertex_count, m1.anim_node_index, m1.basis_floats),
                 (m2.stream_flags, m2.max_bones_per_tri, m2.max_bones_per_vert, m2.bone_effector,
                  m2.reindexed_bones_flag, m2.use_matrix_palettes, m2.min_bone, m2.max_bone,
                  m2.reindexed_bones, m2.undup_vertex_count, m2.anim_node_index, m2.basis_floats))
            d.eq(w + " vertex count", len(m1.vertices), len(m2.vertices))
            d.eq(w + " indices", m1.indices, m2.indices)
            d.eq(w + " bone sets", [(s.first_vertex, s.vertex_count, s.bones, s.index_into_index_buffer)
                                    for s in m1.bone_sets],
                 [(s.first_vertex, s.vertex_count, s.bones, s.index_into_index_buffer) for s in m2.bone_sets])
            d.eq(w + " dup map", m1.dup_map, m2.dup_map)
            d.eq(w + " raw vertices", m1.raw_vertex_data, m2.raw_vertex_data)
            if len(m1.vertices) != len(m2.vertices):
                continue
            pos = nrm = uv = wt = bas = 0.0
            idx_bad = 0
            for v1, v2 in zip(m1.vertices, m2.vertices):
                if v1.position is not None:
                    pos = max(pos, vdiff(v1.position, v2.position))
                    nrm = max(nrm, vdiff(v1.normal, v2.normal))
                    wt = max(wt, vdiff(v1.weights, v2.weights))
                    idx_bad += v1.bone_indices != v2.bone_indices
                for u1, u2 in zip(v1.uvs, v2.uvs):
                    uv = max(uv, vdiff(u1, u2))
                if len(v1.uvs) != len(v2.uvs):
                    uv = float("inf")
                if v1.basis:
                    bas = max(bas, vdiff(v1.basis, v2.basis))
            d.num("pos", w + " positions", pos, TOL_POS)
            d.num("normal", w + " normals", nrm, TOL_DIR)
            d.num("uv", w + " uvs", uv, TOL_UV)
            d.num("weight", w + " blend weights", wt, 1e-6)
            d.num("basis", w + " basis", bas, TOL_DIR)
            d.eq(w + " bone indices differ", 0, idx_bad)
    d.eq("animations", [x.name for x in a.animations], [x.name for x in b.animations])
    for x, y in zip(a.animations, b.animations):
        d.eq(x.name + " header", (x.dims, x.compression, x.interpolation_time),
             (y.dims, y.compression, y.interpolation_time))
        d.eq(x.name + " key times", [k.time for k in x.keyframes], [k.time for k in y.keyframes])
        d.eq(x.name + " key strings", [k.command for k in x.keyframes], [k.command for k in y.keyframes])
        if len(x.keyframes) != len(y.keyframes) or len(x.tracks) != len(y.tracks):
            continue
        pe = qe = ve = 0.0
        for ni, (t1, t2) in enumerate(zip(x.tracks, y.tracks)):
            if t1.is_vertex_anim or t2.is_vertex_anim:
                d.eq("%s node %d vertex anim" % (x.name, ni), t1.is_vertex_anim, t2.is_vertex_anim)
                for f1, f2 in zip(t1.vertex_frames, t2.vertex_frames):
                    ve = max(ve, vdiff(f1, f2))
                continue
            for k in range(len(x.keyframes)):
                p1, q1 = x.key_transform(ni, k)
                p2, q2 = y.key_transform(ni, k)
                pe = max(pe, vdiff(p1, p2))
                qe = max(qe, qdiff(q1, q2))
        # 16-bit positions are 1/16 unit steps: a re-encoded value can land
        # on the neighbouring step only if the float drifted past a half.
        d.num("anim_pos", x.name + " positions", pe, TOL_POS if x.compression != ltb.ANCMPRS_REL_16 else 0.0626)
        d.num("anim_rot", x.name + " rotations", qe, TOL_QUAT)
        d.num("vertex_anim", x.name + " vertex frames", ve, TOL_POS)
    d.eq("sockets", [(s.node, s.name, s.scale) for s in a.sockets], [(s.node, s.name, s.scale) for s in b.sockets])
    for s, t in zip(a.sockets, b.sockets):
        d.num("socket_pos", s.name + " position", vdiff(s.position, t.position), TOL_POS)
        d.num("socket_rot", s.name + " rotation", qdiff(s.rotation, t.rotation), TOL_QUAT)
    d.eq("weight sets", [(w.name, w.weights) for w in a.weight_sets], [(w.name, w.weights) for w in b.weight_sets])
    d.eq("child models", a.child_models, b.child_models)
    d.eq("anim bindings", [[(x.name, x.dims, x.translation) for x in l] for l in a.anim_bindings],
         [[(x.name, x.dims, x.translation) for x in l] for l in b.anim_bindings])
    d.eq("child relations", a.child_relations, b.child_relations)
    return d


def main(argv):
    args = argv[argv.index("--") + 1:] if "--" in argv else []
    limit = every = None
    paths = []
    i = 0
    while i < len(args):
        if args[i] == "--limit":
            limit = int(args[i + 1]); i += 2
        elif args[i] == "--every":
            every = int(args[i + 1]); i += 2
        else:
            paths.append(Path(args[i])); i += 1
    if not paths:
        paths = [ASSETS]
    files = []
    for p in paths:
        files += sorted(p.rglob("*.ltb")) if p.is_dir() else [p]
    files = [f for f in files if not ltb.is_chunked(f.read_bytes()[:16])]
    if every:
        files = files[::every]
    if limit:
        files = files[:limit]

    io_scene_lithtech.register()
    tmp = Path(tempfile.mkdtemp(prefix="lt_rt_ltb_"))
    failed = 0
    worst = {}
    t0 = time.time()
    for n, f in enumerate(files, 1):
        purge()
        rel = f.relative_to(ASSETS) if ASSETS in f.parents else f
        if os.environ.get("LT_VERBOSE"):
            print("TRY", rel, flush=True)
        try:
            a = ltb.read_file(f)
            arm = import_ltb.load(bpy.context, str(f), child_animations=False, import_textures=False)
            out = tmp / "out.ltb"
            export_ltb.save(bpy.context, str(out), armature=arm)
            b = ltb.read_file(out)
            d = compare(a, b)
        except Exception:
            failed += 1
            print("FAIL %s: exception" % rel)
            traceback.print_exc()
            continue
        for k, v in d.max.items():
            if v[0] > worst.get(k, (0.0,))[0]:
                worst[k] = (v[0], "%s: %s" % (rel, v[1]))
        if d.errors:
            failed += 1
            print("FAIL %s: %d problems; first: %s" % (rel, len(d.errors), "; ".join(d.errors[:3])))
        if n % 100 == 0:
            print("... %d/%d, %d failed, %.0fs" % (n, len(files), failed, time.time() - t0))
    print()
    print("worst differences:")
    for k in sorted(worst):
        print("  %-12s %-10.3g %s" % (k, worst[k][0], worst[k][1]))
    print("%d models, %d failed, %.0fs" % (len(files), failed, time.time() - t0))
    return 1 if failed else 0


if __name__ == "__main__":
    code = main(sys.argv)
    sys.stdout.flush()
    os._exit(code)
