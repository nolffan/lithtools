#!/usr/bin/env python3
"""Runs the original LithRez (under wine) and ours on the same trees and
compares the archives they create, with the clock-stamped time fields
masked. Meant for the talon build image (wine), with /work on a tmpfs:

    compare_original.py <our lithrez> <work dir on tmpfs> <LithRez.exe>... [-- <rez to unpack as a tree>...]

On tmpfs a directory lists newest first, so each tree is created in reverse
of Windows' order (upper-cased names) to make wine list it as NTFS would.
"""
import os
import shutil
import struct
import subprocess
import sys


def ntfs_key(name):
    return name.upper().encode("utf-8")


def make_tree(dest, entries):
    """entries: {relative path: bytes or None for a directory}."""
    tree = {}
    for p, data in entries.items():
        parts = p.split("/")
        node = tree
        for d in parts[:-1]:
            node = node.setdefault(d, {})
        if data is None:
            node.setdefault(parts[-1], {})
        else:
            node[parts[-1]] = data

    def build(path, node):
        os.makedirs(path, exist_ok=True)
        for name in sorted(node, key=ntfs_key, reverse=True):
            child = node[name]
            full = os.path.join(path, name)
            if isinstance(child, dict):
                os.mkdir(full)
            else:
                with open(full, "wb") as f:
                    f.write(child)
        for name in sorted(node, key=ntfs_key, reverse=True):
            if isinstance(node[name], dict):
                build(os.path.join(path, name), node[name])

    shutil.rmtree(dest, ignore_errors=True)
    build(dest, tree)


def mask_times(x):
    x = bytearray(x)
    struct.pack_into("<I", x, 127 + 12, 0)   # root dir time (garbage in 1.10)
    struct.pack_into("<I", x, 127 + 20, 0)   # time
    ver, rp, rs = struct.unpack_from("<3I", x, 127)

    def walk(pos, size):
        o, end = pos, pos + size
        while o < end:
            k, = struct.unpack_from("<I", x, o)
            o += 4
            if k == 1:
                dp, ds, dt = struct.unpack_from("<3I", x, o)
                struct.pack_into("<I", x, o + 8, 0)
                o = x.index(b"\0", o + 12) + 1
                if ds:
                    walk(dp, ds)
            else:
                struct.pack_into("<I", x, o + 8, 0)
                nk, = struct.unpack_from("<I", x, o + 20)
                o = x.index(b"\0", o + 24) + 1
                o = x.index(b"\0", o) + 1 + 4 * nk

    walk(rp, rs)
    return bytes(x)


def synthetic():
    e = {
        "a.txt": b"lower a",
        "B.TXT": b"upper b",
        "_under.dat": b"underscore",
        "Zed.abc": b"zed",
        "123.txt": b"id from name",
        "0042.dtx": b"leading zeros",
        "empty.wav": b"",
        "readme.longext": b"long extension",
        "noext": b"no extension",
        "two.dtx": b"two dtx",
        "two.abc": b"two abc",
        "with space.txt": b"space",
        "tilde~1.txt": b"tilde",
        "x.y.z": b"two dots",
        "emptydir": None,
        "chars/models/hero.abc": b"m" * 3000,
        "chars/skins/hero.dtx": b"s" * 5000,
        "chars/skins/HERO_HEAD.dtx": b"h" * 700,
        "worlds/dm_one.dat": b"w" * 1234,
        "worlds/sub/deeper/file.txt": b"deep",
    }
    for i in range(40):   # fill the hash bins
        e["many/f%02d_%s.txt" % (i, "x" * (i % 23))] = bytes([65 + i % 26]) * (i + 1)
    for i in range(12):
        e["dirs/d%s/f.txt" % ("y" * i)] = b"d"
    return e


def from_rez(ours, rez, work):
    out = os.path.join(work, "unpack")
    shutil.rmtree(out, ignore_errors=True)
    subprocess.run([ours, "x", rez, out], check=True, capture_output=True)
    entries = {}
    for root, dirs, files in os.walk(out):
        for d in dirs:
            entries[os.path.relpath(os.path.join(root, d), out)] = None
        for f in files:
            p = os.path.join(root, f)
            entries[os.path.relpath(p, out)] = open(p, "rb").read()
    shutil.rmtree(out)
    return entries


def main():
    args = sys.argv[1:]
    rezzes = []
    if "--" in args:
        i = args.index("--")
        args, rezzes = args[:i], args[i + 1:]
    ours, work, exes = args[0], args[1], args[2:]
    trees = [("synthetic", synthetic())] + [(os.path.basename(r), from_rez(ours, r, work)) for r in rezzes]
    bad = 0
    for name, entries in trees:
        tree = os.path.join(work, "tree")
        make_tree(tree, entries)
        mine = os.path.join(work, "OURS.REZ")
        outs = {}
        for compat in ("1.00", "1.10"):
            subprocess.run([ours, "c", mine, tree, "--time", "0", "--compat", compat], check=True,
                           capture_output=True)
            outs[compat] = mask_times(open(mine, "rb").read())
        for exe in exes:
            ver = subprocess.run(["strings", "-n", "8", exe], capture_output=True, text=True).stdout
            compat = "1.00" if ("LITHREZ 1.00" in ver or "LITHREZ 1.01" in ver) else "1.10"
            m = outs[compat]
            theirs = os.path.join(work, "THEIRS.REZ")
            if os.path.exists(theirs):
                os.remove(theirs)
            r = subprocess.run(["wine", exe, "c", "Z:" + theirs.replace("/", "\\"), "Z:" + tree.replace("/", "\\")],
                               capture_output=True, text=True, errors="replace")
            if not os.path.exists(theirs):
                print("%-22s %-40s did not create a file: %s" % (name, exe[-40:], r.stdout[-300:]))
                bad += 1
                continue
            t = mask_times(open(theirs, "rb").read())
            if t == m:
                print("%-22s %-40s identical to --compat %s (times masked), %d bytes" % (name, exe[-40:], compat, len(t)))
            else:
                bad += 1
                i = next((i for i in range(min(len(t), len(m))) if t[i] != m[i]), min(len(t), len(m)))
                print("%-22s %-40s DIFFER at %d (sizes %d ours %d)" % (name, exe[-40:], i, len(t), len(m)))
                shutil.copy(theirs, os.path.join(os.environ.get("KEEP", work), "keep-%s-%s.rez" % (name, os.path.basename(os.path.dirname(exe)))))
                shutil.copy(mine, os.path.join(os.environ.get("KEEP", work), "keep-%s-ours.rez" % name))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
