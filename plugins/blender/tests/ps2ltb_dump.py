#!/usr/bin/env python3
"""Prints what tools/lithrez's `ps2ltb_verify --dump` prints, from
lithtech.ps2ltb, to check the C++ reader against this one:

    ps2ltb_dump.py <file.ltb> [--against <ps2ltb_verify --dump output>]

With --against, compares the two token by token (numbers within 1e-4,
everything else exactly) and exits non-zero on a difference.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "io_scene_lithtech"))
from lithtech import ps2ltb  # noqa: E402


def apply(m, v, w):
    return [m[4 * r] * v[0] + m[4 * r + 1] * v[1] + m[4 * r + 2] * v[2] + m[4 * r + 3] * w for r in range(3)]


def lod_mesh(m, lod):
    """(vertices [(position, normal, [(node, w)])], triangles [(i, j, k, (s, t) of the first corner)]),
    as ps2ltb::to_abc makes them."""
    verts, ids_per_strip = [], []
    if lod.kind == ps2ltb.SKEL_MESH:
        for u in lod.uniques:
            ws = [(lod.bone_map[b // 4], w / 4096.0) for w, b in zip(u.weights, u.bones)
                  if b != 0xFF and w and b // 4 < len(lod.bone_map)]
            verts.append((u.position[:3], u.normal[:3], ws))
        ids_per_strip = [[v.unique for v in s.vertices] for s in lod.strips()]
    else:
        bind = m.nodes[lod.node].bind_matrix
        seen = {}
        for s in lod.strips():
            ids = []
            for v in s.vertices:
                key = struct.pack("<8f", *v.position, *v.normal)
                if key not in seen:
                    seen[key] = len(verts)
                    verts.append((apply(bind, v.position, 1.0), apply(bind, v.normal, 0.0), [(lod.node, 1.0)]))
                ids.append(seen[key])
            ids_per_strip.append(ids)
    tris = []
    for s, ids in zip(lod.strips(), ids_per_strip):
        for t in s.triangles():
            i = [ids[c] for c in t]
            if len(set(i)) == 3:
                v = s.vertices[t[0]]
                q = v.stq[2] or 1.0
                tris.append((i[0], i[1], i[2], (v.stq[0] / q, v.stq[1] / q)))
    return verts, tris


def dump(path):
    m = ps2ltb.read_file(path)
    out = ["nodes %d pieces %d anims %d sockets %d children %d mult %d" % (
        len(m.nodes), len(m.pieces), len(m.animations), len(m.sockets), len(m.child_models or []), m.hash_mult)]
    for pi, p in enumerate(m.pieces):
        out.append("piece %d %s tex %d" % (pi, m.piece_name(p), p.texture))
        for li, lod in enumerate(p.lods):
            verts, tris = lod_mesh(m, lod)
            out.append(" lod %d kind %d verts %d tris %d" % (li, lod.kind, len(verts), len(tris)))
            for vi, (pos, nrm, ws) in enumerate(verts[:3]):
                out.append("  v%d %.6f %.6f %.6f n %.6f %.6f %.6f%s" % (
                    vi, *pos, *nrm, "".join(" %d:%.6f" % (n, w) for n, w in ws)))
            for ti, (a, b, c, uv) in enumerate(tris[:2]):
                out.append("  t%d %d %d %d uv %.6f %.6f" % (ti, a, b, c, uv[0], uv[1]))
    for ai, a in enumerate(m.animations[:3]):
        out.append("anim %d %s keys %d" % (ai, m.animation_name(a), len(a.keyframes)))
        for n in range(min(len(m.nodes), 4)):
            for k in range(min(len(a.keyframes), 2)):
                p, q = a.key_transform(n, k)
                out.append("  n%d k%d %.6f %.6f %.6f %.6f %.6f %.6f %.6f" % (n, k, *p, *q))
    for s in m.sockets:
        out.append("socket %s node %d" % (m.socket_name(s), s.node))
    return out


def same(a, b):
    ta, tb = a.split(), b.split()
    if len(ta) != len(tb):
        return False
    for x, y in zip(ta, tb):
        if x == y:
            continue
        try:
            if abs(float(x) - float(y)) > 1e-4 * max(1.0, abs(float(x))):
                return False
        except ValueError:
            return False
    return True


def main():
    lines = dump(sys.argv[1])
    if "--against" in sys.argv:
        other = Path(sys.argv[sys.argv.index("--against") + 1]).read_text().splitlines()
        bad = [(i, a, b) for i, (a, b) in enumerate(zip(lines, other)) if not same(a, b)]
        if len(lines) != len(other):
            bad.append((min(len(lines), len(other)), "%d lines" % len(lines), "%d lines" % len(other)))
        for i, a, b in bad[:5]:
            print("line %d:\n  py:  %s\n  c++: %s" % (i, a, b))
        print("%s: %d lines, %s" % (sys.argv[1], len(lines), "same" if not bad else "%d differ" % len(bad)))
        sys.exit(1 if bad else 0)
    print("\n".join(lines))


if __name__ == "__main__":
    main()
