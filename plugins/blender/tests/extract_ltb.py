#!/usr/bin/env python3
"""Extract every shipped .ltb model (NOLF2, Contract J.A.C.K., Tron 2.0,
and the tools' and Jupiter builds' sample trees) into the gitignored asset
cache, one copy of each distinct file.

    extract_ltb.py [outdir]     (default ~/src/lithtools/out/assets/ltb)

Writes <outdir>/<game>/<rez stem>/<path inside the rez> (loose files under
<game>/loose/...). Render style .ltb files (LTB file type 5) are skipped:
they aren't models. Game data is never committed.
"""
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path.home() / "src/tools"))
from rezextract import Rez  # noqa: E402

HOME = Path.home()
EXT = HOME / "src/lithtools/extracted"


def rezzes(*globs):
    out = []
    for base, pat in globs:
        out += sorted(p for p in Path(base).glob(pat) if p.is_file())
    return out


SOURCES = {
    "nolf2-usa": rezzes((HOME / "src/rez/games/nolf2-usa", "*.[rR][eE][zZ]"),
                        (HOME / "src/rez/games/nolf2-mac-usa", "**/*.[rR][eE][zZ]")),
    "cjack-us": rezzes((HOME / "src/rez/games/contract-jack-us", "*.[rR][eE][zZ]")),
    "tron2-usa": rezzes((EXT / "tron2-usa", "**/*.[rR][eE][zZ]")),
    "tools": rezzes((EXT / "nolf2-toolkit", "**/*.[rR][eE][zZ]"), (EXT / "cjack-tools", "**/*.[rR][eE][zZ]"),
                    (EXT / "tron2-tools-official", "**/*.[rR][eE][zZ]"), (EXT / "jupiter-b51", "**/*.[rR][eE][zZ]"),
                    (EXT / "jupiter-b69", "**/*.[rR][eE][zZ]")),
}
LOOSE = [EXT / "nolf2-toolkit", EXT / "cjack-tools", EXT / "tron2-tools-official", EXT / "jupiter-b51",
         EXT / "jupiter-b69"]


def is_model(head):
    return head[:1] == b"\x01" or head[:11] == b"\x09\x00LTBHeader"


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HOME / "src/lithtools/out/assets/ltb"
    seen = set()
    for game, rzs in SOURCES.items():
        n = 0
        for rp in rzs:
            rez = Rez(str(rp))
            for p, pos, sz in rez.walk():
                if not p.lower().endswith(".ltb"):
                    continue
                rez.f.seek(pos)
                data = rez.f.read(sz)
                h = hashlib.sha1(data).digest()
                if h in seen or not is_model(data):
                    continue
                seen.add(h)
                dest = out / game / rp.stem / p
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
                n += 1
        print(f"{game}: {n} distinct .ltb models")
    n = 0
    for base in LOOSE:
        for f in sorted(base.rglob("*")):
            if f.suffix.lower() != ".ltb" or not f.is_file():
                continue
            data = f.read_bytes()
            h = hashlib.sha1(data).digest()
            if h in seen or not is_model(data):
                continue
            seen.add(h)
            dest = out / "loose" / f.relative_to(EXT)
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
            n += 1
    print(f"loose: {n} distinct .ltb models")


if __name__ == "__main__":
    main()
