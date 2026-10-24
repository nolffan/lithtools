"""Imports each shipped .abc into Blender, exports it again and compares the
two models field by field (Blender stores floats as float32 and keeps bone
matrices only up to its roll maths, so numbers are compared within a
tolerance, everything else exactly).

    blender -b --factory-startup --python tests/blender_roundtrip.py -- \
        [--limit N] [--every K] [--child] [paths or directories...]

Defaults to every model under out/assets/abc. Exits non-zero if any model
fails.
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
from io_scene_lithtech import export_abc, import_abc  # noqa: E402
from io_scene_lithtech.lithtech import abc  # noqa: E402

ASSETS = HERE.parents[2] / "out/assets/abc"

TOL_POS = 2e-3       # LithTech units (models are ~10..1000 units across)
TOL_DIR = 1e-4       # normals, matrix rotation parts
TOL_UV = 1e-6
TOL_QUAT = 1e-5      # 1 - |dot|


def purge():
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.armatures, bpy.data.actions,
                 bpy.data.materials, bpy.data.images, bpy.data.collections):
        bpy.data.batch_remove(list(coll))


def vdiff(a, b):
    return max(abs(x - y) for x, y in zip(a, b)) if a else 0.0


def qdiff(a, b):
    return 1.0 - abs(sum(x * y for x, y in zip(a, b)))


class Diff:
    def __init__(self):
        self.errors = []
        self.max = {}

    def eq(self, what, a, b):
        if a != b:
            self.errors.append("%s: %r != %r" % (what, a, b))

    def num(self, kind, what, d, tol):
        if d > self.max.get(kind, (0.0,))[0]:
            self.max[kind] = (d, what)
        if d > tol or math.isnan(d):
            self.errors.append("%s: off by %g" % (what, d))


def sums_to_one(v):
    return abs(sum(w.bias for w in v.weights) - 1.0) < 1e-4


def compare(a, b):
    d = Diff()
    d.eq("version", a.version, b.version)
    d.eq("command_string", a.command_string, b.command_string)
    d.eq("lod_distances", a.lod_distances, b.lod_distances)
    d.eq("header_reserved", a.header_reserved, b.header_reserved)
    d.num("radius", "internal_radius", abs(a.internal_radius - b.internal_radius),
          max(TOL_POS, a.internal_radius * 0.05))

    an, bn = a.nodes, b.nodes
    d.eq("node names", [n.name for n in an], [n.name for n in bn])
    d.eq("node flags", [n.flags for n in an], [n.flags for n in bn])
    d.eq("node parents", a.node_parents(), b.node_parents())
    for x, y in zip(an, bn):
        rot = max(abs(x.bind_matrix[i] - y.bind_matrix[i]) for i in (0, 1, 2, 4, 5, 6, 8, 9, 10))
        d.num("bind_rot", "bind %s rotation" % x.name, rot, TOL_DIR)
        d.num("bind_pos", "bind %s position" % x.name,
              vdiff([x.bind_matrix[i] for i in (3, 7, 11)], [y.bind_matrix[i] for i in (3, 7, 11)]),
              TOL_POS)

    d.eq("piece names", [p.name for p in a.pieces], [p.name for p in b.pieces])
    for p, q in zip(a.pieces, b.pieces):
        d.eq(p.name + " texture", p.texture_index, q.texture_index)
        d.eq(p.name + " specular", (p.specular_power, p.specular_scale, p.lod_weight, p.reserved),
             (q.specular_power, q.specular_scale, q.lod_weight, q.reserved))
        d.eq(p.name + " lods", len(p.lods), len(q.lods))
        for li, (l1, l2) in enumerate(zip(p.lods, q.lods)):
            w = "%s lod%d" % (p.name, li)
            d.eq(w + " vertex count", len(l1.vertices), len(l2.vertices))
            d.eq(w + " triangles", [t.indices for t in l1.triangles], [t.indices for t in l2.triangles])
            if len(l1.vertices) != len(l2.vertices):
                continue
            uv = max((vdiff(sum(t1.uvs, ()), sum(t2.uvs, ()))
                      for t1, t2 in zip(l1.triangles, l2.triangles)), default=0.0)
            d.num("uv", w + " uv", uv, TOL_UV)
            pos = nrm = wl = 0.0
            for v1, v2 in zip(l1.vertices, l2.vertices):
                pos = max(pos, vdiff(v1.location, v2.location))
                nrm = max(nrm, vdiff(v1.normal, v2.normal))
                w1 = sorted((x.node, x.bias) for x in v1.weights if x.bias > 1e-6)
                w2 = sorted((x.node, x.bias) for x in v2.weights if x.bias > 1e-6)
                if True:
                    if [n for n, _ in w1] != [n for n, _ in w2] or \
                            vdiff([b for _, b in w1], [b for _, b in w2]) > 1e-6:
                        d.errors.append("%s weights %r != %r" % (w, w1, w2))
                        break
                    if True:
                        s1 = sorted(((x.node, x.location) for x in v1.weights if x.bias > 1e-6))
                        s2 = sorted(((x.node, x.location) for x in v2.weights if x.bias > 1e-6))
                        wl = max(wl, max(vdiff(l1_, l2_) for (_, l1_), (_, l2_) in zip(s1, s2)))
                if v1.replacement != v2.replacement:
                    d.errors.append("%s replacement" % w)
                    break
            d.num("pos", w + " positions", pos, TOL_POS)
            d.num("normal", w + " normals", nrm, TOL_DIR)
            d.num("weight_loc", w + " weight locations", wl, TOL_POS)

    d.eq("animation names", [x.name for x in a.animations], [x.name for x in b.animations])
    for x, y in zip(a.animations, b.animations):
        d.eq(x.name + " header", (x.dims, x.modeledit_weight_set, x.interpolation_time),
             (y.dims, y.modeledit_weight_set, y.interpolation_time))
        d.eq(x.name + " key times", [k.time for k in x.keyframes], [k.time for k in y.keyframes])
        d.eq(x.name + " key strings", [k.command for k in x.keyframes],
             [k.command for k in y.keyframes])
        if len(x.keyframes) != len(y.keyframes):
            continue
        pe = qe = 0.0
        for t1, t2 in zip(x.node_transforms, y.node_transforms):
            for (p1, q1), (p2, q2) in zip(t1, t2):
                pe = max(pe, vdiff(p1, p2))
                ln = math.sqrt(sum(c * c for c in q1)) or 1.0
                qe = max(qe, qdiff([c / ln for c in q1], q2))
        d.num("anim_pos", x.name + " positions", pe, TOL_POS)
        d.num("anim_rot", x.name + " rotations", qe, TOL_QUAT)

    d.eq("socket names", [s.name for s in a.sockets], [s.name for s in b.sockets])
    for s, t in zip(a.sockets, b.sockets):
        d.eq(s.name + " node", s.node, t.node)
        d.num("socket_pos", s.name + " position", vdiff(s.position, t.position), TOL_POS)
        ln = math.sqrt(sum(c * c for c in s.rotation)) or 1.0
        d.num("socket_rot", s.name + " rotation", qdiff([c / ln for c in s.rotation], t.rotation),
              TOL_QUAT)

    d.eq("weight sets", [(w.name, w.weights) for w in a.weight_sets],
         [(w.name, w.weights) for w in b.weight_sets])
    d.eq("child models", [(c.name, c.save_index, c.relation) for c in a.child_models],
         [(c.name, c.save_index, c.relation) for c in b.child_models])
    d.eq("anim bindings", a.anim_bindings, b.anim_bindings)
    return d


def main(argv):
    args = argv[argv.index("--") + 1:] if "--" in argv else []
    limit = every = None
    child = False
    paths = []
    i = 0
    while i < len(args):
        if args[i] == "--limit":
            limit = int(args[i + 1]); i += 2
        elif args[i] == "--every":
            every = int(args[i + 1]); i += 2
        elif args[i] == "--child":
            child = True; i += 1
        else:
            paths.append(Path(args[i])); i += 1
    if not paths:
        paths = [ASSETS]
    files = []
    for p in paths:
        files += sorted(p.rglob("*.abc")) if p.is_dir() else [p]
    if every:
        files = files[::every]
    if limit:
        files = files[:limit]

    io_scene_lithtech.register()
    tmp = Path(tempfile.mkdtemp(prefix="lt_rt_"))
    failed = 0
    worst = {}
    t0 = time.time()
    for n, f in enumerate(files, 1):
        purge()
        rel = f.relative_to(ASSETS) if ASSETS in f.parents else f
        try:
            a = abc.read_file(f)
            arm = import_abc.load(bpy.context, str(f), child_animations=child,
                                  import_textures=False)
            out = tmp / "out.abc"
            export_abc.save(bpy.context, str(out), armature=arm, normalize_weights=False)
            b = abc.read_file(out)
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
        if n % 50 == 0:
            print("... %d/%d, %d failed, %.0fs" % (n, len(files), failed, time.time() - t0))
    print()
    print("worst differences:")
    for k in sorted(worst):
        print("  %-11s %-10.3g %s" % (k, worst[k][0], worst[k][1]))
    print("%d models, %d failed, %.0fs" % (len(files), failed, time.time() - t0))
    return 1 if failed else 0


if __name__ == "__main__":
    code = main(sys.argv)
    sys.stdout.flush()
    os._exit(code)
