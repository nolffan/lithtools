#!/usr/bin/env python3
"""Packs the Blender extension as an installable zip.

    package.py <version> <out.zip>

<version> goes into blender_manifest.toml (Blender wants semantic
versions: "1.2.0", or "0.1.0-nightly+abc1234" style for a build). The zip
has the manifest at its root, as `blender --command extension build` makes
it; __pycache__ is left out. Symbolic links are stored as the files they
point to.
"""
import os
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "io_scene_lithtech")


def main():
    version, out = sys.argv[1], sys.argv[2]
    if not re.fullmatch(r"\d+\.\d+\.\d+([-+][0-9A-Za-z.+-]+)?", version):
        sys.exit("version must look like 1.2.3 or 1.2.3-nightly+abc1234: %r" % version)
    manifest = open(os.path.join(SRC, "blender_manifest.toml"), encoding="utf-8").read()
    manifest, n = re.subn(r'(?m)^version = ".*"$', 'version = "%s"' % version, manifest)
    if n != 1:
        sys.exit("no version line in blender_manifest.toml")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("blender_manifest.toml", manifest)
        for root, dirs, files in os.walk(SRC, followlinks=True):
            dirs[:] = sorted(d for d in dirs if d != "__pycache__")
            for f in sorted(files):
                if f.endswith(".pyc") or f == "blender_manifest.toml" and root == SRC:
                    continue
                p = os.path.join(root, f)
                z.write(p, os.path.relpath(p, SRC))
    print(out)


if __name__ == "__main__":
    main()
