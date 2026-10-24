#!/usr/bin/env python3
"""Round-trips every .dtx we have through lithtech.dtx.

Sources (read in place, nothing extracted): every .rez under
~/src/rez/games and ~/src/lithtools/extracted, and the loose .dtx files
under ~/src/lithtools/extracted (the Jupiter builds, the Tron 2.0 tools).
Identical files are tested once.

For each file: read, write, compare the bytes; decode every level of every
cube face to RGBA. For uncompressed 32-bit files, also rebuild the mips
from the top level with build_mipmaps() and count the files whose stored
mips it reproduces. Then encode a few decoded textures in each format and
check the round trip through write/read/decode.

    python3 roundtrip_dtx.py [-j N] [--no-decode] [--limit N]
"""
import argparse
import collections
import hashlib
import multiprocessing
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, os.path.expanduser("~/src/tools"))

from lithtech import dtx  # noqa: E402
from rezextract import Rez  # noqa: E402

REZ_ROOTS = [Path("~/src/rez/games").expanduser(),
             Path("~/src/lithtools/extracted").expanduser()]
LOOSE_ROOTS = [Path("~/src/lithtools/extracted").expanduser()]


def source_name(path):
    p = str(path)
    for r in REZ_ROOTS:
        if p.startswith(str(r)):
            return Path(p[len(str(r)) + 1:]).parts[0]
    return "?"


def corpus():
    """Yield (source, name, bytes) for every .dtx, each content once."""
    seen = set()
    for root in REZ_ROOTS:
        for rp in sorted(root.rglob("*")):
            if rp.suffix.lower() != ".rez" or not rp.is_file():
                continue
            try:
                rz = Rez(str(rp))
            except Exception:
                continue
            for name, pos, size in rz.walk():
                if not name.lower().endswith(".dtx"):
                    continue
                rz.f.seek(pos)
                data = rz.f.read(size)
                h = hashlib.sha1(data).digest()
                if h not in seen:
                    seen.add(h)
                    yield source_name(rp), f"{rp.name}:{name}", data
            rz.f.close()
    for root in LOOSE_ROOTS:
        for p in sorted(root.rglob("*")):
            if p.suffix.lower() != ".dtx" or not p.is_file():
                continue
            data = p.read_bytes()
            h = hashlib.sha1(data).digest()
            if h not in seen:
                seen.add(h)
                yield source_name(p), str(p.relative_to(root)), data


def check(job):
    source, name, data, decode, cross = job
    r = {"source": source, "name": name, "size": len(data)}
    if not data:
        r.update(status="empty")
        return r
    try:
        t = dtx.read(data)
    except Exception as e:
        ver = int.from_bytes(data[4:8], "little", signed=True) if len(data) >= 8 else None
        r.update(status="read-fail", error=str(e), version=ver)
        return r
    r.update(version=t.version, bpp=t.bpp_name, cube=t.is_cubemap,
             sections=tuple(s.type for s in t.sections), mips=t.mip_count,
             w=t.width, h=t.height, trailing=len(t.trailing),
             truncated=t.truncated)
    try:
        out = dtx.write(t)
    except Exception as e:
        r.update(status="write-fail", error=str(e))
        return r
    if out != bytes(data):
        n = next((i for i, (a, b) in enumerate(zip(out, data)) if a != b),
                 min(len(out), len(data)))
        r.update(status="mismatch", error=f"first difference at {n}")
        return r
    r["status"] = "ok"
    if decode:
        try:
            faces = 6 if t.is_cubemap else 1
            for face in range(faces):
                for m in range(t.mip_count):
                    w, h, px = t.decode(m, face)
                    if len(px) != w * h * 4:
                        raise ValueError(f"mip {m} decoded to {len(px)} bytes")
            r["decoded"] = True
            if cross and dtx._np is not None and dtx.is_compressed(t.bpp):
                # numpy and plain decoders must agree byte for byte.
                r["cross_ok"] = all(
                    dtx._decode_dxt(t.bpp, *t.mip_size(m), t.mips[m]) ==
                    dtx._decode_dxt_np(t.bpp, *t.mip_size(m), t.mips[m])
                    for m in range(t.mip_count))
            if t.bpp == dtx.BPP_32 and t.mip_count > 1:
                w, h, top = t.decode(0)
                rebuilt = dtx.build_mipmaps(w, h, dtx.rgba_to_bgra(top), t.mip_count)
                r["mips_rebuilt"] = rebuilt[1:] == t.mips[1:]
        except NotImplementedError as e:
            r["decoded"] = f"unsupported: {e}"
        except Exception as e:
            r["decoded"] = f"fail: {e}"
    return r


def encode_checks(samples):
    """Encode decoded textures to each format and back; report the error."""
    lines = []
    for name, t in samples:
        w, h, px = t.decode(0)
        for bpp in (dtx.BPP_32, dtx.BPP_S3TC_DXT1, dtx.BPP_S3TC_DXT3,
                    dtx.BPP_S3TC_DXT5):
            new = dtx.from_rgba(w, h, px, bpp=bpp)
            back = dtx.read(dtx.write(new))
            assert dtx.write(back) == dtx.write(new)
            w2, h2, px2 = back.decode(0)
            # Color error where the source is opaque enough to be drawn
            # (DXT1 makes the rest transparent black), and alpha error.
            ce = [abs(px[i + c] - px2[i + c]) for i in range(0, len(px), 4)
                  if px[i + 3] >= 128 for c in range(3)]
            ae = [abs(px[i] - px2[i]) for i in range(3, len(px), 4)]
            if bpp == dtx.BPP_S3TC_DXT1:
                ae = [abs((px[i] >= 128) * 255 - px2[i]) for i in range(3, len(px), 4)]
            if bpp == dtx.BPP_32:
                assert px2 == px, "BPP_32 encode is not lossless"
            lines.append(
                f"  {name} {w}x{h} -> {dtx.BPP_NAMES[bpp]:4} {back.mip_count} mips: "
                f"color mean {sum(ce) / max(len(ce), 1):.2f} max {max(ce, default=0)}, "
                f"alpha mean {sum(ae) / max(len(ae), 1):.2f} max {max(ae, default=0)}")
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-j", type=int, default=os.cpu_count())
    ap.add_argument("--no-decode", action="store_true")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--cross", type=int, default=20, metavar="N",
                    help="with numpy, check its S3TC decode against the "
                    "plain one on every Nth file (0: never)")
    args = ap.parse_args()

    t0 = time.time()
    jobs = []
    for i, (src, name, data) in enumerate(corpus()):
        if args.limit and i >= args.limit:
            break
        jobs.append((src, name, data, not args.no_decode,
                     bool(args.cross) and i % args.cross == 0))
    print(f"{len(jobs)} distinct .dtx files ({time.time() - t0:.0f}s to gather)")

    with multiprocessing.Pool(args.j) as pool:
        results = pool.map(check, jobs, chunksize=16)

    status = collections.Counter(r["status"] for r in results)
    by_kind = collections.Counter(
        (r["source"], r.get("version"), r.get("bpp"), r["status"]) for r in results)
    print("\nstatus:", dict(status))
    print("\nby source, version, format, status:")
    for k, n in sorted(by_kind.items(), key=lambda kv: tuple(map(str, kv[0]))):
        print(f"  {n:6}  {k[0]:24} v{k[1]} {k[2] or '-':5} {k[3]}")
    secs = collections.Counter(r.get("sections") for r in results if r.get("sections"))
    print("\nsection lists:", dict(secs))
    print("cube maps:", sum(1 for r in results if r.get("cube")))
    print("trailing bytes:", sum(1 for r in results if r.get("trailing")))
    for r in results:
        if r.get("truncated"):
            print(f"truncated (pixel data short, kept): {r['source']} {r['name']}")
        if r["status"] == "empty":
            print(f"empty file (0 bytes, not a DTX): {r['source']} {r['name']}")

    if not args.no_decode:
        dec = collections.Counter(
            "ok" if r.get("decoded") is True else str(r.get("decoded"))
            for r in results if r["status"] == "ok")
        print("\ndecode:", dict(dec))
        reb = [r["mips_rebuilt"] for r in results if "mips_rebuilt" in r]
        print(f"BPP_32 files whose mips build_mipmaps() reproduces: "
              f"{sum(reb)} of {len(reb)}")
        cross = [r for r in results if "cross_ok" in r]
        if dtx._np is None:
            print("numpy not importable: plain decoder only")
        else:
            print(f"S3TC files where numpy and plain decoders agree: "
                  f"{sum(r['cross_ok'] for r in cross)} of {len(cross)}")
            for r in cross:
                if not r["cross_ok"]:
                    print(f"  decoders differ: {r['source']} {r['name']}")

    bad = [r for r in results if r["status"] not in ("ok", "empty") or
           (not args.no_decode and r["status"] == "ok" and
            r.get("decoded") is not True)]
    if bad:
        print(f"\n{len(bad)} problems (first 30):")
        for r in bad[:30]:
            print(f"  {r['source']} {r['name']}: {r['status']} "
                  f"{r.get('error', '')} {r.get('decoded', '')}")

    # Encoding: a few small textures of each stored format.
    samples, want = [], collections.Counter()
    for src, name, data, _, _ in jobs:
        try:
            t = dtx.read(data)
        except Exception:
            continue
        if t.width * t.height <= 128 * 128 and want[t.bpp] < 1 and not t.is_cubemap:
            want[t.bpp] += 1
            samples.append((name.split(":")[-1], t))
    print("\nencode round trips:")
    print("\n".join(encode_checks(samples)))
    print(f"\n{time.time() - t0:.0f}s")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
