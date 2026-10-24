#!/usr/bin/env python3
"""Compares lithtech::sound's decoding with FFmpeg's, on a sample of the
shipped sounds (a few of every codec, channel count and rate in each
archive) and on synthetic files FFmpeg encodes in the codecs no game uses.

    compare_sound.py <build dir> <work dir> <file.rez>...

PCM, float, A-law and mu-law must match FFmpeg sample for sample. MP3
decoders differ in rounding: they must agree within a signal-to-noise ratio
of 60 dB, with the same sample counts.

IMA ADPCM is checked against a reference decoder here instead: FFmpeg
computes each step as ((2n + 1) * step) >> 3, while the IMA recommendation
(and Microsoft's codec) add shifted steps bit by bit, which rounds
differently. So our output must equal the bitwise reference exactly, and
FFmpeg's must equal the same reference with FFmpeg's formula, which shows
the formula is the only difference.

Needs ffmpeg and the build's lithrez and sound_decode_all.
"""
import array
import collections
import math
import struct
import subprocess
import sys
from pathlib import Path

PER_GROUP = 3
EXACT = {"PCM", "IMA ADPCM", "IEEE float", "A-law", "mu-law"}

IMA_INDEX = [-1, -1, -1, -1, 2, 4, 6, 8] * 2
IMA_STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80,
            88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
            598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
            3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635,
            13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767]


def ima_reference(wav, ffmpeg_formula):
    """Decodes an IMA ADPCM .wav, bitwise (the reference) or FFmpeg's way."""
    o, fmt, data = 12, None, None
    while o + 8 <= len(wav):
        cid, ln = struct.unpack_from("<4sI", wav, o)
        if cid == b"fmt " and fmt is None:
            fmt = wav[o + 8:o + 8 + ln]
        elif cid == b"data" and data is None:
            data = wav[o + 8:o + 8 + ln]
        o += 8 + ln + (ln & 1)
    _tag, ch, _rate, _bps, align, _bits = struct.unpack_from("<HHIIHH", fmt)
    out = array.array("h")
    for b0 in range(0, len(data) - 4 * ch + 1, align):
        blk = data[b0:b0 + align]
        st = [list(struct.unpack_from("<hB", blk, 4 * c)) for c in range(ch)]
        frames = [[s[0] for s in st]]
        groups = (len(blk) - 4 * ch) // (4 * ch)
        cols = [[] for _ in range(ch)]
        q = 4 * ch
        for _g in range(groups):
            for c in range(ch):
                for byte in blk[q:q + 4]:
                    for nib in (byte & 15, byte >> 4):
                        pred, idx = st[c]
                        s = IMA_STEP[idx]
                        if ffmpeg_formula:
                            diff = ((2 * (nib & 7) + 1) * s) >> 3
                        else:
                            diff = (s >> 3) + (s if nib & 4 else 0) + (s >> 1 if nib & 2 else 0) + (s >> 2 if nib & 1 else 0)
                        pred = max(-32768, min(32767, pred - diff if nib & 8 else pred + diff))
                        st[c] = [pred, max(0, min(88, idx + IMA_INDEX[nib]))]
                        cols[c].append(pred)
                q += 4
        frames += list(zip(*cols))
        for f in frames:
            out.extend(f)
    return out


def run(*cmd, **kw):
    return subprocess.run(cmd, check=True, capture_output=True, **kw)


def ffmpeg_pcm(src, channels):
    out = run("ffmpeg", "-v", "error", "-i", str(src), "-f", "s16le", "-ac", str(channels), "-").stdout
    return array.array("h", out)


def ours(build, src, path, raw):
    line = run(str(build / "tests/sound_decode_all"), "--dump", str(src), path, str(raw)).stdout.decode().split()
    return line, array.array("h", raw.read_bytes())


def compare(a, b):
    n = min(len(a), len(b))
    diffs = [abs(a[k] - b[k]) for k in range(n)]
    worst = max(diffs, default=0)
    noise = sum(d * d for d in diffs)
    signal = sum(b[k] * b[k] for k in range(n))
    snr = math.inf if noise == 0 else 10 * math.log10(signal / noise) if signal else -math.inf
    return worst, snr, len(a) - len(b)


def check(name, codec, a, b, results, wav=None):
    if codec == "IMA ADPCM":
        # ours against the bitwise reference; FFmpeg against its own formula
        ref = ima_reference(wav, False)
        if ima_reference(wav, True) != b:
            print(f"MISMATCH {name}: FFmpeg isn't the reference with its own formula")
            results[codec].append((False, 0, 0))
            return
        b = ref
    worst, snr, dlen = compare(a, b)
    ok = dlen == 0 and (worst == 0 if codec in EXACT else snr > 60)
    results[codec].append((ok, worst, snr))
    if not ok:
        print(f"MISMATCH {name}: {codec}, length {dlen:+d}, worst {worst}, SNR {snr:.1f} dB")


def main():
    build, work, rezzes = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3:]
    work.mkdir(parents=True, exist_ok=True)
    listing = run(str(build / "tests/sound_decode_all"), "--list", *rezzes).stdout.decode().splitlines()
    groups = collections.defaultdict(list)
    for line in listing:
        rez, path, codec, ch, rate, frames = line.split("\t")
        groups[(rez, codec, ch, rate)].append((rez, path, int(ch)))
    picks = []
    for key in sorted(groups):
        items = groups[key]
        step = max(1, len(items) // PER_GROUP)
        picks += items[::step][:PER_GROUP]
    results = collections.defaultdict(list)
    src, raw = work / "sound.wav", work / "ours.raw"
    for rez, path, ch in picks:
        src.write_bytes(run(str(build / "lithrez"), "cat", rez, path).stdout)
        (codec, *_), a = ours(build, rez, path, raw)
        codec = {"MPEG": "MPEG layer 3", "IMA": "IMA ADPCM", "IEEE": "IEEE float"}.get(codec, codec)
        check(f"{rez} {path}", codec, a, ffmpeg_pcm(src, ch), results, src.read_bytes())

    # Codecs and layouts the games don't use, encoded by FFmpeg.
    synth = [("MS ADPCM stereo", ["-c:a", "adpcm_ms", "-ac", "2"], "Microsoft ADPCM"),
             ("IMA ADPCM stereo", ["-c:a", "adpcm_ima_wav", "-ac", "2"], "IMA ADPCM"),
             ("A-law", ["-c:a", "pcm_alaw"], "A-law"), ("mu-law", ["-c:a", "pcm_mulaw"], "mu-law"),
             ("PCM 8-bit stereo", ["-c:a", "pcm_u8", "-ac", "2"], "PCM"),
             ("PCM 24-bit", ["-c:a", "pcm_s24le"], "PCM"), ("PCM 32-bit", ["-c:a", "pcm_s32le"], "PCM"),
             ("float 32", ["-c:a", "pcm_f32le"], "IEEE float"), ("float 64", ["-c:a", "pcm_f64le"], "IEEE float"),
             ("PCM 6 channels (extensible)", ["-c:a", "pcm_s16le", "-ac", "6"], "PCM")]
    for name, args, codec in synth:
        f = work / "synth.wav"
        run("ffmpeg", "-v", "error", "-y", "-f", "lavfi", "-i",
            "sine=frequency=440:duration=1.3:sample_rate=22050,volume=0.8", "-f", "lavfi", "-i",
            "anoisesrc=d=1.3:r=22050:a=0.1", "-filter_complex", "amix=inputs=2:normalize=0", *args, str(f))
        ch = int(args[args.index("-ac") + 1]) if "-ac" in args else 1
        (got, *_), a = ours(build, f, "-", raw)
        check(name, codec, a, ffmpeg_pcm(f, ch), results, f.read_bytes())

    bad = 0
    for codec, rs in sorted(results.items()):
        fails = sum(1 for r in rs if not r[0])
        bad += fails
        worst = max(r[1] for r in rs)
        snr = min(r[2] for r in rs)
        print(f"{codec:16} {len(rs):4} files, {len(rs) - fails:4} ok; worst difference {worst}, lowest SNR "
              f"{'exact' if snr == math.inf else f'{snr:.1f} dB'}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
