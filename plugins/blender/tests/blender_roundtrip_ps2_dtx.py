"""PS2 textures through Blender: import, export, compare.

    blender -b --factory-startup --python tests/blender_roundtrip_ps2_dtx.py -- [--every K]

For every K-th PS2 texture under ~/src/lithtools/out/assets/ps2 (default
40): import it (textures.load_image, palette alpha Auto) and export it
unchanged, which must give the file's bytes; save the .blend, open it
again, export again, the same. For each BPP_32P one, also:
  * repaint pixels with other colours of its palette: the export keeps
    the palette and reads back as the edited image exactly;
  * paint a colour that isn't in the palette: the export is quantised
    again, and reads back exactly when there are still 256 colours or
    fewer (else within a reported error).
Then export a few PC true-colour textures as 32P and report the error.
"""
import os
import sys
import tempfile
from pathlib import Path

import bpy

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from io_scene_lithtech import textures  # noqa: E402
from io_scene_lithtech.lithtech import dtx  # noqa: E402

ROOT = Path("~/src/lithtools/out/assets/ps2").expanduser()
args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
every = int(args[args.index("--every") + 1]) if "--every" in args else 40

files = sorted(f for f in ROOT.rglob("*") if f.suffix.lower() == ".dtx" and f.is_file()
               and f.read_bytes()[:4] == b"\0\0\0\0")[::every]
tmp = Path(tempfile.mkdtemp())
counts = dict(n=0, same=0, same_reload=0, kept=0, kept_ok=0, requant=0, requant_ok=0)
errs = []
fails = []


def set_rgba(img, w, h, rgba):
    img.pixels.foreach_set(textures._rgba_to_pixels(w, h, rgba))


imported = []
for i, f in enumerate(files):
    try:
        img = textures.load_image(str(f))
        img.use_fake_user = True
        out = tmp / ("a%d.dtx" % i)
        textures.save_image(img, str(out))
        counts["n"] += 1
        counts["same"] += out.read_bytes() == f.read_bytes()
        imported.append((f, img.name))
        t = dtx.read(f.read_bytes())
        if t.bpp != dtx.BPP_32P:
            continue
        w, h, rgba = textures._pixels_to_rgba(img)
        rgba = bytearray(rgba)
        pal = t.palette(ps2_alpha=img["lt_ps2_alpha"])
        used = sorted(set(t.mips[0]))
        # repaint a stripe with another colour of the palette
        if len(used) > 1:
            c = pal[used[-1] * 4:used[-1] * 4 + 4]
            for x in range(0, w * h, 7):
                rgba[x * 4:x * 4 + 4] = c
            set_rgba(img, w, h, bytes(rgba))
            out = tmp / ("b%d.dtx" % i)
            textures.save_image(img, str(out))
            t2 = dtx.read(out.read_bytes())
            counts["kept"] += 1
            back = t2.decode(0, ps2_alpha=img["lt_ps2_alpha"])[2]
            counts["kept_ok"] += (t2.find_section("PALLETE32").data == t.find_section("PALLETE32").data
                                  and back == textures._pixels_to_rgba(img)[2])
        # a colour that isn't in the palette
        new = bytes((1, 2, 3, 255))
        while new in {pal[k:k + 4] for k in range(0, 1024, 4)}:
            new = bytes((new[0] + 1, 2, 3, 255))
        for x in range(0, w * h, 11):
            rgba[x * 4:x * 4 + 4] = new
        set_rgba(img, w, h, bytes(rgba))
        want = textures._pixels_to_rgba(img)[2]
        out = tmp / ("c%d.dtx" % i)
        textures.save_image(img, str(out))
        back = dtx.read(out.read_bytes()).decode(0, ps2_alpha=img["lt_ps2_alpha"])[2]
        counts["requant"] += 1
        if back == want:
            counts["requant_ok"] += 1
        else:
            errs.append(sum(abs(a - b) for a, b in zip(back, want)) / len(want))
        # back to the imported pixels for the reload test
        set_rgba(img, w, h, t.decode(0, ps2_alpha=img["lt_ps2_alpha"])[2])
    except Exception as e:  # noqa: BLE001
        fails.append((f, e))

blend = tmp / "ps2.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
bpy.ops.wm.open_mainfile(filepath=str(blend))
for i, (f, name) in enumerate(imported):
    out = tmp / ("r%d.dtx" % i)
    textures.save_image(bpy.data.images[name], str(out))
    counts["same_reload"] += out.read_bytes() == f.read_bytes()

pc_err = []
sys.path.insert(0, os.path.expanduser("~/src/tools"))
from rezextract import Rez  # noqa: E402
rz = Rez(os.path.expanduser("~/src/rez/games/nolf-usa/NOLF.REZ"))
n = 0
for name, pos, size in rz.walk():
    if not name.lower().startswith("tex/") or not name.lower().endswith(".dtx") or n >= 8:
        continue
    rz.f.seek(pos)
    p = tmp / ("pc%d.dtx" % n)
    p.write_bytes(rz.f.read(size))
    img = textures.load_image(str(p))
    w, h, want = textures._pixels_to_rgba(img)
    if w > 256:
        continue
    out = tmp / ("pc%d-32p.dtx" % n)
    textures.save_image(img, str(out), bpp=dtx.BPP_32P)
    t = dtx.read(out.read_bytes())
    back = t.decode(0, ps2_alpha=True)[2]
    rgb = sum(abs(back[k] - want[k]) for k in range(len(want)) if k % 4 != 3) / (len(want) * 3 / 4)
    alpha = sum(abs(back[k] - want[k]) for k in range(3, len(want), 4)) / (len(want) / 4)
    pc_err.append((name, t.mip_count, rgb, alpha, max(want[3::4]) == 0))
    n += 1

print("PS2 textures: %(n)d imported and exported unchanged, %(same)d byte-identical "
      "(%(same_reload)d after saving and reopening the .blend)" % counts)
print("  repainted with their own palette's colours: %(kept_ok)d of %(kept)d keep it and read back exactly" % counts)
print("  painted with a new colour: %(requant_ok)d of %(requant)d read back exactly" % counts
      + ("; the rest within %.2f per channel" % max(errs) if errs else ""))
for name, mips, rgb, alpha, no_alpha in pc_err:
    print("  PC %s as 32P (%d mips): mean error %.2f in RGB, %.2f in alpha%s" % (
        name, mips, rgb, alpha, " (its alpha is all zero, unused: as 32P that reads opaque)" if no_alpha else ""))
for f, e in fails:
    print("  FAIL", f, e)
ok = not fails and counts["same"] == counts["n"] == counts["same_reload"] and counts["kept_ok"] == counts["kept"]
print("ALL OK" if ok else "FAILED")
