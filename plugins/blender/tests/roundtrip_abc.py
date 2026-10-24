#!/usr/bin/env python3
"""Round-trip every shipped .abc: read it, write it back, compare bytes.

    roundtrip_abc.py [dir ...]   (default ~/src/lithtools/out/assets/abc,
                                  filled by extract_abc.py)

Reports, per version, how many files came back byte-identical, and lists
every failure. Also checks the stored header counts against the counts
compute_header_counts derives (reported, not a failure).
"""
import collections
import sys
import traceback
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from lithtech import abc  # noqa: E402


def first_diff(a, b):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i
    return min(len(a), len(b))


def main():
    dirs = [Path(a) for a in sys.argv[1:]] or [Path.home() / "src/lithtools/out/assets/abc"]
    files = sorted(p for d in dirs for p in d.rglob("*") if p.suffix.lower() == ".abc")
    if not files:
        sys.exit("no .abc files found; run extract_abc.py first")
    ok = collections.Counter()
    total = collections.Counter()
    failures = []
    count_mismatch = collections.Counter()
    count_examples = {}
    rebuilt = collections.Counter()
    for p in files:
        data = p.read_bytes()
        ver = int.from_bytes(data[12:16], "little") if data[2:8] == b"Header" else -1
        total[ver] += 1
        try:
            m = abc.read(data)
            out = abc.write(m)
        except Exception as e:
            failures.append((p, ver, "%s: %s" % (type(e).__name__, e)))
            if len(failures) <= 3:
                traceback.print_exc()
            continue
        if out == data:
            ok[ver] += 1
        else:
            i = first_diff(out, data)
            failures.append((p, ver, "differs at 0x%x (%d vs %d bytes)" % (i, len(out), len(data))))
        # Also rebuild with the stored counts dropped: write() derives them.
        stored = m.header_counts
        m.header_counts = None
        if abc.write(m) == data:
            rebuilt[ver] += 1
        m.header_counts = stored
        calc = abc.compute_header_counts(m)
        for name, a, b in zip(abc.HEADER_COUNT_NAMES, m.header_counts, calc):
            if a != b:
                count_mismatch[name] += 1
                count_examples.setdefault(name, (p.name, a, b))
    print("version  identical  rebuilt-counts / files")
    for v in sorted(total):
        print("  v%-5d %9d %15d / %d" % (v, ok[v], rebuilt[v], total[v]))
    print("  all    %9d %15d / %d" % (sum(ok.values()), sum(rebuilt.values()), sum(total.values())))
    if count_mismatch:
        print("header counts that compute_header_counts does not reproduce:")
        for name, n in count_mismatch.most_common():
            f, a, b = count_examples[name]
            print("  %-15s %4d files (e.g. %s: stored %d, computed %d)" % (name, n, f, a, b))
    for p, v, why in failures:
        print("FAIL v%d %s: %s" % (v, p, why))
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
