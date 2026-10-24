"""Imports each PS2 model (.ltb file type 2) into Blender, exports it again
and compares the two field by field: hashes, counts, strips, weights and
other integers exactly; positions, normals, UVs and matrices within a
tolerance; animation keys within one step of their quantisation. Also counts
the files that come back byte for byte.

    blender -b --factory-startup --python tests/blender_roundtrip_ps2ltb.py -- \\
        [--limit N] [--every K] [paths or directories...]

Defaults to every PS2 model under out/assets/ps2 (both discs). Exits
non-zero if any model fails.
"""

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
from io_scene_lithtech import export_ps2ltb, import_ps2ltb  # noqa: E402
from io_scene_lithtech.lithtech import ps2ltb  # noqa: E402

ASSETS = HERE.parents[2] / "out/assets/ps2"
TOL = 2e-3
TOL_DIR = 2e-3
TOL_UV = 1e-5


def purge():
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.armatures, bpy.data.actions,
                 bpy.data.materials, bpy.data.images, bpy.data.collections):
        try:
            bpy.data.batch_remove(list(coll))
        except Exception:
            pass


def vd(a, b):
    return max((abs(x - y) for x, y in zip(a, b)), default=0.0) if len(a) == len(b) else float("inf")


def compare(a, b):
    err = []

    def chk(cond, what):
        if not cond:
            err.append(what)
    chk(a.command == b.command and a.radius == b.radius and a.hash_mult == b.hash_mult and
        a.unknown == b.unknown and a.lod_distances == b.lod_distances, "header")
    chk(a.counts == b.counts, "counts %s / %s" % (a.counts, b.counts))
    chk(len(a.nodes) == len(b.nodes), "node count")
    for x, y in zip(a.nodes, b.nodes):
        chk((x.name, x.name_hash, x.child_count, x.index, x.flags) == (y.name, y.name_hash, y.child_count, y.index, y.flags),
            "node %s" % x.name)
        chk(vd(x.bind_matrix, y.bind_matrix) < TOL, "node %s bind" % x.name)
    chk([(w.name_hash, len(w.weights)) for w in a.weight_sets] == [(w.name_hash, len(w.weights)) for w in b.weight_sets]
        and all(vd(x.weights, y.weights) < 1e-6 for x, y in zip(a.weight_sets, b.weight_sets)), "weight sets")
    chk(a.child_models == b.child_models, "child models")
    chk(len(a.pieces) == len(b.pieces), "piece count")
    for pi, (p, q) in enumerate(zip(a.pieces, b.pieces)):
        chk((p.name_hash, p.colors, p.texture, p.unknown, len(p.lods)) ==
            (q.name_hash, q.colors, q.texture, q.unknown, len(q.lods)), "piece %d" % pi)
        for li, (l, k) in enumerate(zip(p.lods, q.lods)):
            where = "piece %d lod %d" % (pi, li)
            chk(l.kind == k.kind and l.node == k.node, where + " kind/node")
            ls, ks = l.strips(), k.strips()
            chk([len(b_.strips) for b_ in l.buffers] == [len(b_.strips) for b_ in k.buffers], where + " buffers")
            chk([(len(s.vertices), s.reversed, s.gif_hi, s.gif_lo_rest) for s in ls] ==
                [(len(s.vertices), s.reversed, s.gif_hi, s.gif_lo_rest) for s in ks], where + " strips")
            if len(err) > 20:
                return err
            for s, t in zip(ls, ks):
                for v, w in zip(s.vertices, t.vertices):
                    if vd(v.position, w.position) > TOL or vd(v.normal, w.normal) > TOL_DIR or \
                            vd(v.stq, w.stq) > TOL_UV or v.extra != w.extra:
                        err.append(where + " strip vertex")
                        break
            if l.kind == ps2ltb.SKEL_MESH:
                chk(l.bone_map == k.bone_map, where + " bone map")
                chk(len(l.uniques) == len(k.uniques), where + " unique count")
                for u, v in zip(l.uniques, k.uniques):
                    if vd(u.position, v.position) > TOL or vd(u.normal, v.normal) > TOL_DIR or \
                            (u.weights, u.bones) != (v.weights, v.bones):
                        err.append(where + " unique vertex")
                        break
    chk(len(a.animations) == len(b.animations), "animation count")
    for ai, (x, y) in enumerate(zip(a.animations, b.animations)):
        where = "animation %d" % ai
        chk((x.name_hash, x.interpolation_time, [(k.time, k.command) for k in x.keyframes]) ==
            (y.name_hash, y.interpolation_time, [(k.time, k.command) for k in y.keyframes]), where + " keys")
        chk(vd(x.dims, y.dims) < 1e-6 and vd(x.translation, y.translation) < 1e-6, where + " dims")
        for ni, ((m1, t1), (m2, t2)) in enumerate(zip(x.tracks, y.tracks)):
            bad = m1 != m2 or len(t1) != len(t2)
            for v, w in zip(t1, t2):
                if v[3] != w[3] or max(abs(c - d) for c, d in zip(v, w)) > 1:
                    # a quaternion may come back negated (the same rotation)
                    if not (v[:4] == w[:4] or max(abs(c - d) for c, d in zip(v[:3], w[:3])) <= 1) or \
                            max(abs(c + d) for c, d in zip(v[4:], w[4:])) > 1:
                        bad = True
            if bad:
                err.append("%s node %d" % (where, ni))
                break
    chk(len(a.sockets) == len(b.sockets), "socket count")
    for x, y in zip(a.sockets, b.sockets):
        chk((x.node, x.name_hash, x.name_hash1, x.pad) == (y.node, y.name_hash, y.name_hash1, y.pad), "socket")
        chk(vd(x.position, y.position) < TOL, "socket position")
        q = sum(c * d for c, d in zip(x.rotation, y.rotation))
        chk(abs(abs(q) - 1.0) < 1e-4, "socket rotation")
    return err


def main(argv):
    limit, every, paths = None, 1, []
    i = 0
    while i < len(argv):
        if argv[i] == "--limit":
            limit = int(argv[i + 1])
            i += 2
        elif argv[i] == "--every":
            every = int(argv[i + 1])
            i += 2
        else:
            paths.append(Path(argv[i]))
            i += 1
    files = []
    for p in paths or [ASSETS]:
        if p.is_dir():
            files += sorted(f for f in p.rglob("*") if f.suffix.lower() == ".ltb")
        else:
            files.append(p)
    files = [f for f in files if ps2ltb.is_ps2_model(f.read_bytes()[:8])][::every]
    if limit:
        files = files[:limit]
    bad = identical = 0
    t0 = time.time()
    tmp = tempfile.mkdtemp()
    for f in files:
        purge()
        data = f.read_bytes()
        try:
            a = ps2ltb.read(data)
            arm = import_ps2ltb.load(bpy.context, str(f), import_textures=False, child_animations=False)
            out = os.path.join(tmp, "out.ltb")
            export_ps2ltb.save(bpy.context, out, armature=arm)
            got = open(out, "rb").read()
            identical += got == data
            err = compare(a, ps2ltb.read(got))
        except Exception:
            err = [traceback.format_exc()]
        if err:
            bad += 1
            print("FAIL %s: %s" % (f, "; ".join(err[:6])))
    print("%d models, %d failed, %d byte for byte, %.0fs" % (len(files), bad, identical, time.time() - t0))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
