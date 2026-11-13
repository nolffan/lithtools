#!/usr/bin/env python3
"""ehsplit.py: an icl object whose EH-framed functions are /Ob0's.

    ehsplit.py <normal.obj> <ob0.obj> [--list <out.txt>]

A stand-in for a difference between our Intel C/C++ 4.0 build and the one
that built NOLF's Processor.exe: Monolith's never inlined anything into a
function with an EH frame, and inlined as usual elsewhere; ours inlines
everywhere, or with /Ob0 nowhere (progress/Processor_nolf_map.md has the
evidence). So each icl source of the processor is compiled twice, as the
project says and with /Ob0 added, and this rewrites the first object (in
place) so that every function EH-framed in the /Ob0 compile has the /Ob0
compile's code, and everything else the normal compile's:

  - a function is EH-framed when its section has a relocation to
    __except_list (icl's frame links itself into fs:0's chain through
    it); only ordinary functions (COMDAT NODUPLICATES) are taken, and
    never a static initializer (one .CRT$XCU names);
  - each one's section is replaced in place by the /Ob0 one, which brings
    the symbols the /Ob0 compile defines in it (its $Bn$EH handler thunk,
    a $$1 body), its relocations remapped into this object;
  - the /Ob0 compile's .data1 (its literals and exception tables, which
    its code names by offset) comes whole, as a second .data1 section;
    any section the taken code needs that this object lacks (an
    out-of-line copy of an inline, a static function our inlining
    compile never emitted) comes too, transitively;
  - every other symbol the taken code names is this object's: by name for
    named symbols, and by section for .data, .bss and the like, which the
    two compiles lay out alike (each split checks they're the same size).

Section order is kept, so the link and its layout are unchanged. The
normal compile's tables for a replaced function stay behind as
unreferenced data. --list writes the replaced functions' names.
"""

import struct
import sys

COMDAT = 0x1000
SEL_NODUP, SEL_ASSOC = 1, 5


class Obj:
    def __init__(self, data):
        self.machine, n, self.stamp, symoff, nsyms, optsz, self.chars = \
            struct.unpack_from("<HHIIIHH", data, 0)
        if optsz:
            raise SystemExit("ehsplit: an object with an optional header")
        self.secs = []
        for i in range(n):
            o = 20 + 40 * i
            vsize, _va, size, raw, rel, _lnp, nrel, _nln, fl = struct.unpack_from("<IIIIIIHHI", data, o + 8)
            self.secs.append(dict(
                name=data[o:o + 8], vsize=vsize, size=size, flags=fl,
                body=data[raw:raw + size] if raw else None,
                relocs=[list(struct.unpack_from("<IIH", data, rel + 10 * k)) for k in range(nrel)]))
        strings = data[symoff + 18 * nsyms:]
        self.syms = []      # [name, value, secnum, type, class, aux bytes] or None for an aux slot
        i = 0
        while i < nsyms:
            e = symoff + 18 * i
            raw = data[e:e + 18]
            value, sec, typ, cls, naux = struct.unpack_from("<IhHBB", raw, 8)
            if raw[:4] == b"\0\0\0\0":
                off = struct.unpack_from("<I", raw, 4)[0]
                name = strings[off:strings.index(b"\0", off)]
            else:
                name = raw[:8].rstrip(b"\0")
            self.syms.append([name, value, sec, typ, cls, data[e + 18:e + 18 + 18 * naux]])
            self.syms.extend([None] * naux)
            i += 1 + naux

    def secname(self, num):
        return self.secs[num - 1]["name"].rstrip(b"\0")

    def is_secsym(self, k):
        s = self.syms[k]
        return s is not None and s[4] == 3 and s[2] > 0 and s[1] == 0 and len(s[5]) == 18 \
            and s[0] == self.secname(s[2])

    def secsym(self, num):
        for k, s in enumerate(self.syms):
            if s is not None and s[2] == num and self.is_secsym(k):
                return k
        return None

    def selection(self, num):
        k = self.secsym(num)
        return self.syms[k][5][14] if k is not None and self.secs[num - 1]["flags"] & COMDAT else None

    def add_symbol(self, s):
        self.syms.append(list(s))
        self.syms.extend([None] * (len(s[5]) // 18))
        return len(self.syms) - 1 - len(s[5]) // 18

    def write(self):
        names = bytearray(4)
        table = bytearray()
        count = 0
        for s in self.syms:
            if s is None:
                continue
            name, value, sec, typ, cls, aux = s
            field = name.ljust(8, b"\0") if len(name) <= 8 else b"\0\0\0\0" + struct.pack("<I", len(names))
            if len(name) > 8:
                names += name + b"\0"
            table += field + struct.pack("<IhHBB", value, sec, typ, cls, len(aux) // 18) + aux
            count += 1 + len(aux) // 18
        struct.pack_into("<I", names, 0, len(names))
        pos = 20 + 40 * len(self.secs)
        headers, body = bytearray(), bytearray()
        for s in self.secs:
            raw = rel = 0
            if s["body"] is not None and s["size"]:
                raw = pos
                body += s["body"]
                pos += s["size"]
            if s["relocs"]:
                rel = pos
                for r in s["relocs"]:
                    body += struct.pack("<IIH", *r)
                pos += 10 * len(s["relocs"])
            if len(s["relocs"]) > 0xffff:
                raise SystemExit("ehsplit: too many relocations in one section")
            headers += s["name"].ljust(8, b"\0") + struct.pack(
                "<IIIIIIHHI", s["vsize"], 0, s["size"], raw, rel, 0, len(s["relocs"]), 0, s["flags"])
        head = struct.pack("<HHIIIHH", self.machine, len(self.secs), self.stamp, pos, count, 0, self.chars)
        return head + bytes(headers) + bytes(body) + bytes(table) + bytes(names)


def functions(o):
    """{name: symbol index} of the function symbols o defines."""
    return {s[0]: k for k, s in enumerate(o.syms)
            if s is not None and s[3] == 0x20 and s[2] > 0 and s[4] in (2, 3)}


def initializers(o):
    out = set()
    for num in range(1, len(o.secs) + 1):
        if o.secname(num).startswith(b".CRT"):
            out |= {o.syms[r[1]][0] for r in o.secs[num - 1]["relocs"]}
    return out


def eh_framed(o, skip):
    exc = {k for k, s in enumerate(o.syms) if s is not None and s[0] == b"__except_list"}
    out = set()
    for name, k in functions(o).items():
        num = o.syms[k][2]
        if name in skip or o.syms[k][1] != 0 or o.selection(num) != SEL_NODUP:
            continue
        if any(r[1] in exc for r in o.secs[num - 1]["relocs"]):
            out.add(name)
    return out


def split(normal, ob0):
    n, b = Obj(normal), Obj(ob0)
    for name in (b".data", b".bss", b".tls$"):
        sn = [s["size"] for s in n.secs if s["name"].rstrip(b"\0") == name]
        sb = [s["size"] for s in b.secs if s["name"].rstrip(b"\0") == name]
        if sn != sb:
            raise SystemExit("ehsplit: %s differs between the compiles" % name.decode())
    nf, bf = functions(n), functions(b)
    take = sorted(eh_framed(b, initializers(n) | initializers(b)) & set(nf))

    # named symbols of n, a definition preferred
    named = {}
    for k, s in enumerate(n.syms):
        if s is None or n.is_secsym(k):
            continue
        if s[0] not in named or (n.syms[named[s[0]]][2] <= 0 < s[2]):
            named[s[0]] = k
    plain = {n.secname(num): num for num in range(1, len(n.secs) + 1)
             if not n.secs[num - 1]["flags"] & COMDAT}
    def comdat_names(o):
        """{COMDAT symbol name: section}: the first symbol after each
        COMDAT section's own."""
        out = {}
        for k, sym in enumerate(o.syms):
            if sym is not None and not o.is_secsym(k) and sym[2] > 0 \
                    and o.secs[sym[2] - 1]["flags"] & COMDAT and sym[2] not in out.values():
                out.setdefault(sym[0], sym[2])
        return out
    ncomdat = comdat_names(n)
    bcomdat = {v: k for k, v in comdat_names(b).items()}

    smap = {}       # b symbol -> n symbol
    secmap = {}     # b section -> n section
    redirect = {}   # n symbol -> n symbol, for n's own relocations
    queue = []

    def bring(bnum):
        """b's section bnum as a new section of n, with its symbols."""
        if bnum in secmap:
            return secmap[bnum]
        s = dict(b.secs[bnum - 1])
        s["relocs"] = [list(r) for r in s["relocs"]]
        n.secs.append(s)
        num = len(n.secs)
        secmap[bnum] = num
        for k, sym in enumerate(b.syms):
            if sym is not None and sym[2] == bnum:
                if sym[4] == 2 and not b.is_secsym(k) and sym[0] in named \
                        and n.syms[named[sym[0]]][2] > 0:
                    # defined here as well (data in a plain section: an
                    # MFC message map): this object's is the one
                    smap[k] = named[sym[0]]
                    continue
                new = [sym[0], sym[1], num, sym[3], sym[4], sym[5]]
                if b.is_secsym(k) and b.selection(bnum) == SEL_ASSOC:
                    queue.append(("assoc", k))
                nk = n.add_symbol(new)
                smap[k] = nk
                old = named.get(sym[0])
                if old is not None and not b.is_secsym(k) and sym[4] == 2:
                    # this object named it without defining it: now it does
                    if n.syms[old][2] <= 0:
                        redirect[old] = nk
                    named[sym[0]] = nk
        queue.append(("relocs", bnum))
        return num

    # the taken functions, in place (one section may define two names)
    seen = set()
    for name in take:
        bk, nk = bf[name], nf[name]
        bnum, num = b.syms[bk][2], n.syms[nk][2]
        if bnum in seen:
            continue
        seen.add(bnum)
        dst, src = n.secs[num - 1], b.secs[bnum - 1]
        dst.update(size=src["size"], vsize=src["vsize"], body=src["body"],
                   relocs=[list(r) for r in src["relocs"]])
        secmap[bnum] = num
        nss, bss = n.secsym(num), b.secsym(bnum)
        aux = bytearray(n.syms[nss][5])
        aux[0:14] = b.syms[bss][5][0:14]      # length, relocations, line numbers, checksum
        n.syms[nss][5] = bytes(aux)
        smap[bss] = nss
        # the symbols of the section: n's, moved to the /Ob0 values where
        # b has the same name, else left at 0 (only the normal compile's
        # own, now unreferenced, tables name them); b's others added
        nsyms_here = {n.syms[k][0]: k for k, s in enumerate(n.syms)
                      if s is not None and s[2] == num and k != nss}
        for k, sym in enumerate(b.syms):
            if sym is None or sym[2] != bnum or k == bss:
                continue
            if sym[0] in nsyms_here:
                nk2 = nsyms_here.pop(sym[0])
                n.syms[nk2][1] = sym[1]
                smap[k] = nk2
            else:
                smap[k] = n.add_symbol([sym[0], sym[1], num, sym[3], sym[4], sym[5]])
        for nk2 in nsyms_here.values():
            n.syms[nk2][1] = 0
        queue.append(("relocs", bnum))

    # the /Ob0 compile's literals and exception tables
    for bnum in range(1, len(b.secs) + 1):
        if b.secname(bnum) == b".data1" and not b.secs[bnum - 1]["flags"] & COMDAT:
            bring(bnum)

    def resolve(k):
        if k in smap:
            return smap[k]
        sym = b.syms[k]
        name, value, sec, typ, cls, aux = sym
        if b.is_secsym(k):
            if sec in secmap:
                smap[k] = n.secsym(secmap[sec])
            elif not b.secs[sec - 1]["flags"] & COMDAT and name in plain:
                smap[k] = n.secsym(plain[name])
            elif b.secs[sec - 1]["flags"] & COMDAT and bcomdat.get(sec) in ncomdat:
                smap[k] = n.secsym(ncomdat[bcomdat[sec]])   # the same COMDAT here
            else:
                bring(sec)
            return smap[k]
        old = named.get(name)
        if old is not None and (n.syms[old][2] > 0 or sec <= 0):
            smap[k] = old           # n has it (or neither defines it)
        elif sec > 0 and b.secs[sec - 1]["flags"] & COMDAT and bcomdat.get(sec) in ncomdat:
            # a COMDAT this object has under the same name: n's (its symbol
            # here is the one the name lookup found, or the section's)
            smap[k] = named.get(name, n.secsym(ncomdat[bcomdat[sec]]))
        elif sec > 0:
            bring(sec)              # only the /Ob0 compile has it
        else:
            smap[k] = n.add_symbol([name, 0, 0, typ, 2, b""])
            named[name] = smap[k]
        return smap[k]

    resolved = set()
    while queue:
        kind, x = queue.pop()
        if kind == "relocs":
            if x in resolved:
                continue
            resolved.add(x)
            for r in n.secs[secmap[x] - 1]["relocs"]:
                r[1] = resolve(r[1])
        else:
            nk = smap[x]
            aux = bytearray(n.syms[nk][5])
            other = struct.unpack_from("<H", aux, 12)[0]
            if other not in secmap:
                bring(other)
            struct.pack_into("<H", aux, 12, secmap[other])
            n.syms[nk][5] = bytes(aux)

    # n's own references to a symbol the /Ob0 code now defines
    if redirect:
        for s in n.secs:
            for r in s["relocs"]:
                r[1] = redirect.get(r[1], r[1])
    return n, take


def main():
    a = sys.argv[1:]
    listing = None
    if "--list" in a:
        i = a.index("--list")
        listing = a[i + 1]
        del a[i:i + 2]
    if len(a) != 2:
        sys.exit(__doc__)
    n, take = split(open(a[0], "rb").read(), open(a[1], "rb").read())
    open(a[0], "wb").write(n.write())
    if listing:
        open(listing, "w").write("".join(t.decode("latin-1") + "\n" for t in take))


if __name__ == "__main__":
    main()
