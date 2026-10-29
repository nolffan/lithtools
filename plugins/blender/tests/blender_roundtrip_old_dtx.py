"""Blood 2 (-2) and KISS Psycho Circus (-3) textures through Blender.

    blender -b --factory-startup --python tests/blender_roundtrip_old_dtx.py -- [--every K]

For every K-th .dtx under ~/src/lithtools/extracted/blood2-demo/rez and
~/src/lithtools/extracted/kiss-psycho-circus/rez (default 25): import it
(textures.load_image), check the image's pixels are the decoded top level,
export it unchanged, which must give the file's bytes, then repaint a pixel
and export again: the old formats have no encoder, so that writes a -5
file, which must read back as the edited image exactly.
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

ROOTS = [Path("~/src/lithtools/extracted/blood2-demo/rez").expanduser(),
         Path("~/src/lithtools/extracted/kiss-psycho-circus/rez").expanduser()]


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    every = int(argv[argv.index("--every") + 1]) if "--every" in argv else 25
    files = sorted(p for r in ROOTS if r.is_dir() for p in r.rglob("*")
                   if p.suffix.lower() == ".dtx")
    if not files:
        print("no -2/-3 textures found under", ", ".join(map(str, ROOTS)))
        return 2
    files = files[::every]
    bad = 0
    counts = {-2: 0, -3: 0}
    with tempfile.TemporaryDirectory() as tmp:
        for i, path in enumerate(files):
            raw = path.read_bytes()
            t = dtx.read(raw)
            counts[t.version] = counts.get(t.version, 0) + 1
            img = textures.load_image(str(path))
            w, h, rgba = t.decode(0)
            if textures._pixels_to_rgba(img)[2] != rgba:   # both top row first
                print("FAIL pixels:", path)
                bad += 1
                continue
            out = os.path.join(tmp, "%d.dtx" % i)
            textures.save_image(img, out)
            if Path(out).read_bytes() != raw:
                print("FAIL unchanged export differs:", path)
                bad += 1
                continue
            # repaint the first pixel: a -5 export, read back exactly
            px = list(img.pixels)
            px[0:4] = [1.0, 0.0, 1.0, 1.0]
            img.pixels.foreach_set(px)
            textures.save_image(img, out)
            t2 = dtx.read(Path(out).read_bytes())
            w2, h2, rgba2 = t2.decode(0)
            if t2.version != -5 or (w2, h2) != (w, h) or \
                    rgba2 != textures._pixels_to_rgba(img)[2]:
                print("FAIL edited export:", path, t2.version, t2.bpp_name)
                bad += 1
            bpy.data.images.remove(img)
    print("%d textures (%d -2, %d -3): %s" % (len(files), counts.get(-2, 0), counts.get(-3, 0),
                                             "%d failed" % bad if bad else "all passed"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
