"""Synthetic DHNP rigid/skeletal imports and conversion, without Blender.

    python3 plugins/blender/tests/dhnp_checks.py
"""

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "io_scene_lithtech"))
from lithtech import abc, dhnp
from lithtech.binio import Reader, Writer


def model():
    root = abc.Node("root", 0, 0, abc.quat_to_matrix((0, 0, 0), (0, 0, 0, 1)))
    root.children.append(abc.Node("child", 1, 0, abc.quat_to_matrix((2, 0, 0), (0, 0, 0, 1))))
    m = abc.Model(root=root)
    vertices = [abc.Vertex([abc.Weight(0, p, 1)], 0, p, (0, 0, 1))
                for p in ((0, 0, 0), (1, 0, 0), (0, 1, 0))]
    lod = abc.LOD([abc.Triangle((0, 1, 2), ((0, 0), (1, 0), (0, 1)))], vertices)
    m.pieces.append(abc.Piece("body", lods=[lod]))
    m.child_models.append(abc.ChildModel("", 0, [((0, 0, 0), (0, 0, 0, 1))] * 2))
    m.animations.append(abc.Animation("base", (1, 1, 1), keyframes=[abc.Keyframe(0, "")],
                                     node_transforms=[[((0, 0, 0), (0, 0, 0, 1))],
                                                      [((2, 0, 0), (0, 0, 0, 1))]]))
    m.anim_bindings.append([abc.AnimBinding("base", (1, 1, 1), (0, 0, 0))])
    return m


def fixture(kind, vertex_format=None, version=12):
    m = model()
    data = abc.write(m)
    sections = {name: data[start:end] for name, start, end in abc.read_chunks(data)}
    if kind == 1:
        p = Writer()
        p.pack("7f", *([0.0] * 7))
        p.raw(bytes(40))
        p.u32(0)
        p.pack("3i", -1, -1, -1)
        p.pack("2fI4I", 5.0, 1.0, 2, 3, 1, 2 if vertex_format is not None else 1,
               2 if vertex_format == 1 else 1)
        if vertex_format is None:
            p.u32(0)
        else:
            p.u8(vertex_format)
            if vertex_format == 1:
                p.raw(bytes(8))
        for v in m.pieces[0].lods[0].vertices:
            p.vec3(v.location)
            if vertex_format == 0:
                p.f32(1.0)
            elif vertex_format == 1:
                p.f32(0.25)
                p.pack("4B", 0, 1, 0, 0)
            p.vec3(v.normal)
        p.pack("6f3H", 0, 0, 1, 0, 0, 1, 0, 1, 2)
        if vertex_format == 0:
            p.pack("I2H4BI", 1, 0, 3, 0, 255, 255, 255, 0)
        pieces = Writer()
        pieces.u32(1)
        pieces.string("body")
        pieces.pack("2I", 4 if vertex_format is None else 5, p.tell())
        pieces.raw(p.getvalue())
        sections["Pieces"] = pieces.getvalue()
    if version == 13:
        h = bytearray(sections["Header"])
        struct.pack_into("<I", h, 0, 13)
        h[56:56] = bytes(4)
        sections["Header"] = bytes(h)
        a = Writer()
        a.u32(1)
        a.vec3((0, 0, 0))
        a.string("base")
        a.pack("i3I", -1, 200, 1, 0)
        a.string("")
        for pos in ((0, 0, 0), (2, 0, 0)):
            a.i32(-1)
            a.vec3(pos)
            a.quat((0, 0, 0, 1))
            a.raw(bytes(8))
        sections["Animation"] = a.getvalue()
    w = Writer()
    w.string("LTBHeader")
    w.pack("I2H4I", 0, kind, 3, 0, 0, 0, 0)
    link = None
    for name in abc.CHUNK_ORDER:
        if link is not None:
            w.patch_u32(link, w.tell())
        w.string(name)
        link = w.tell()
        w.u32(0xFFFFFFFF)
        w.raw(sections[name])
    return w.getvalue()


def main():
    for kind, fmt in ((4, None), (1, None), (1, 0), (1, 1)):
        for version in (12, 13):
            data = fixture(kind, fmt, version)
            assert dhnp.is_model(data)
            m = dhnp.read(data)
            assert m.source_version == version
            assert [n.name for n in m.nodes] == ["root", "child"]
            assert len(m.animations[0].node_transforms) == 2
            lod = m.pieces[0].lods[0]
            assert lod.triangles[0].indices == (0, 1, 2)
            for v in lod.vertices:
                assert max(abs(a - b) for a, b in zip(abc.skinned_position(m, v), v.location)) < 1e-6
            converted = abc.read(abc.write(m))
            assert converted.pieces[0].lods[0].vertices == lod.vertices
    bad = bytearray(fixture(1))
    start = next(start for name, start, _ in abc.read_chunks(bad, 35) if name == "Pieces")
    r = Reader(bad, start)
    r.u32()
    r.string()
    struct.pack_into("<I", bad, r.pos, 7)
    try:
        dhnp.read(bad)
    except abc.ABCError:
        pass
    else:
        raise AssertionError("unsupported DHNP mesh was accepted")
    print("DHNP rigid, skeletal, version 13 and ABC conversion checks passed")


if __name__ == "__main__":
    main()
