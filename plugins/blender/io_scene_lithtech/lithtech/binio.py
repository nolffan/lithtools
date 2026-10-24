"""Little-endian readers and writers for LithTech files.

Standard library only. Strings are LithTech's ILTStream strings: a uint16
length and that many bytes, no terminator, decoded as Latin-1 so every byte
round-trips.
"""
import struct

_S = {}


def _st(fmt):
    s = _S.get(fmt)
    if s is None:
        s = _S[fmt] = struct.Struct("<" + fmt)
    return s


class Reader:
    def __init__(self, data, pos=0):
        self.data = data
        self.pos = pos

    def unpack(self, fmt):
        s = _st(fmt)
        v = s.unpack_from(self.data, self.pos)
        self.pos += s.size
        return v

    def u8(self):
        return self.unpack("B")[0]

    def u16(self):
        return self.unpack("H")[0]

    def u32(self):
        return self.unpack("I")[0]

    def i32(self):
        return self.unpack("i")[0]

    def f32(self):
        return self.unpack("f")[0]

    def vec3(self):
        return self.unpack("3f")

    def quat(self):
        return self.unpack("4f")

    def raw(self, n):
        v = self.data[self.pos:self.pos + n]
        if len(v) != n:
            raise EOFError("read past end of data")
        self.pos += n
        return bytes(v)

    def string(self):
        n = self.u16()
        return self.raw(n).decode("latin-1")


class Writer:
    def __init__(self):
        self.buf = bytearray()

    def tell(self):
        return len(self.buf)

    def pack(self, fmt, *v):
        self.buf += _st(fmt).pack(*v)

    def u8(self, v):
        self.pack("B", v)

    def u16(self, v):
        self.pack("H", v)

    def u32(self, v):
        self.pack("I", v)

    def i32(self, v):
        self.pack("i", v)

    def f32(self, v):
        self.pack("f", v)

    def vec3(self, v):
        self.pack("3f", *v)

    def quat(self, v):
        self.pack("4f", *v)

    def raw(self, b):
        self.buf += b

    def string(self, s):
        b = s.encode("latin-1")
        self.u16(len(b))
        self.buf += b

    def patch_u32(self, pos, v):
        _st("I").pack_into(self.buf, pos, v)

    def getvalue(self):
        return bytes(self.buf)
