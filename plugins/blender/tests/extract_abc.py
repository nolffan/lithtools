#!/usr/bin/env python3
"""Extract every shipped .abc model into the gitignored asset cache.

    extract_abc.py [outdir]     (default ~/src/lithtools/out/assets/abc)

Writes <outdir>/<game>/<rez stem>/<path inside the rez>. Game data is
never committed; out/ is ignored at the top of the repository.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path.home() / "src/tools"))
from rezextract import Rez  # noqa: E402

HOME = Path.home()
SOURCES = {
    "nolf-usa-goty": sorted((HOME / "src/rez/games/nolf-usa-goty").glob("*.[rR][eE][zZ]")),
    "nolf-tools-001": [HOME / "src/lithtools/extracted/nolf-tools-001/zip/Tools/NOLFTOOLS001.REZ"],
    "avp2": sorted((HOME / "src/lithtools/extracted/avp2/cabs").glob("*.REZ")),
    "avp2-ph": sorted((HOME / "src/lithtools/extracted/avp2-ph-us/game").rglob("*.[rR][eE][zZ]")),
}


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HOME / "src/lithtools/out/assets/abc"
    for game, rezzes in SOURCES.items():
        n = 0
        for rp in rezzes:
            if not rp.exists():
                continue
            rez = Rez(str(rp))
            for p, pos, sz in rez.walk():
                if not p.lower().endswith(".abc"):
                    continue
                dest = out / game / rp.stem / p
                dest.parent.mkdir(parents=True, exist_ok=True)
                rez.f.seek(pos)
                dest.write_bytes(rez.f.read(sz))
                n += 1
        print(f"{game}: {n} .abc files")


if __name__ == "__main__":
    main()
