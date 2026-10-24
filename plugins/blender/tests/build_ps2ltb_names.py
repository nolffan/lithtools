#!/usr/bin/env python3
"""Builds io_scene_lithtech/lithtech/ps2ltb_names.txt: the names that give
back the hashes PS2 models store (pieces, animations, weight sets,
sockets).

    build_ps2ltb_names.py <ps2 dir>... --exe <SLUS/SLES file>... --abc <dir>...

Candidates, in the order that wins a collision (the nolfsrc ps2 branch's
ps2model.collect_names): every name in the PC .abc models, then identifiers
in the PS2 executables and attribute files (and their %d forms 0-9), then
numbered weapon animations. Only candidates that match a hash in some PS2
model under <ps2 dir> are kept, so the list is short and holds nothing but
names. Prints how many hashes each kind resolves.
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "io_scene_lithtech"))
from lithtech import abc, ps2ltb  # noqa: E402

OUT = Path(__file__).resolve().parents[1] / "io_scene_lithtech/lithtech/ps2ltb_names.txt"


def candidates(abc_dirs, texts):
    names, seen = [], set()

    def add(n):
        n = n.strip()
        if n and n not in seen and len(n) < 64:
            seen.add(n)
            names.append(n)
    for d in abc_dirs:
        for f in sorted(Path(d).rglob("*")):
            if f.suffix.lower() != ".abc":
                continue
            try:
                m = abc.read(f.read_bytes())
            except Exception:
                continue
            for n in ([x.name for x in m.nodes] + [p.name for p in m.pieces] + [a.name for a in m.animations] +
                      [s.name for s in m.sockets] + [w.name for w in m.weight_sets]):
                add(n)
    tok = re.compile(rb"[A-Za-z_+][A-Za-z0-9_+&\-. ]{1,40}")
    for data in texts:
        for t in tok.findall(data):
            t = t.decode("latin-1")
            for part in [t] + re.split(r"[ .]", t):
                add(part)
                if "%d" in part:
                    for i in range(10):
                        add(part.replace("%d", str(i)))
    for b in ("Idle_", "Fire", "PreFire", "PostFire", "Fire_"):
        for i in range(10):
            add(b + str(i))
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ps2", nargs="+")
    ap.add_argument("--exe", nargs="*", default=[])
    ap.add_argument("--abc", nargs="*", default=[])
    a = ap.parse_args()
    models = []
    texts = [Path(e).read_bytes() for e in a.exe]
    for d in a.ps2:
        for f in sorted(Path(d).rglob("*")):
            if not f.is_file():
                continue
            if f.suffix.lower() == ".txt" and f.parent.name.lower() == "attributes":
                texts.append(f.read_bytes())
            elif f.suffix.lower() == ".ltb":
                data = f.read_bytes()
                if ps2ltb.is_ps2_model(data):
                    models.append(ps2ltb.read(data))
    cands = candidates(a.abc, texts)
    # every (hash, multiplier) a model asks about, by kind
    wanted = {}
    for m in models:
        for kind, hs in (("piece", [p.name_hash for p in m.pieces]),
                         ("animation", [x.name_hash for x in m.animations]),
                         ("weight set", [w.name_hash for w in m.weight_sets]),
                         ("socket", [s.name_hash for s in m.sockets])):
            for h in hs:
                wanted.setdefault((h, m.hash_mult), set()).add(kind)
        for s in m.sockets:
            wanted.setdefault((s.name_hash1, 1), set()).add("socket")
    mults = sorted({k[1] for k in wanted})
    keep = []
    for n in cands:
        if any((ps2ltb.name_hash(n, mu), mu) in wanted for mu in mults):
            keep.append(n)
    names = ps2ltb.Names(keep)
    stats = {}
    for m in models:
        for kind, items, f in (("piece", m.pieces, m.piece_name), ("animation", m.animations, m.animation_name),
                               ("weight set", m.weight_sets, m.weight_set_name), ("socket", m.sockets, m.socket_name)):
            for x in items:
                st = stats.setdefault(kind, [0, 0])
                st[1] += 1
                st[0] += not f(x, names).startswith("#")
    for k, (r, n) in stats.items():
        print("%-10s %6d of %6d resolve" % (k, r, n))
    with open(OUT, "w", encoding="latin-1", newline="\n") as fh:
        fh.write("# Names for the hashes in PS2 NOLF models (lithtech/ps2ltb.py); earlier lines\n"
                 "# win a collision. Made by tests/build_ps2ltb_names.py.\n")
        for n in keep:
            fh.write(n + "\n")
    print("%d names -> %s" % (len(keep), OUT))


if __name__ == "__main__":
    main()
