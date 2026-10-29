"""LithTech .dtx textures (version -5: NOLF, AvP2, AvP2 Primal Hunt, NOLF2,
Contract J.A.C.K., Tron 2.0; and the older -2, Blood 2, and -3, KISS Psycho
Circus).

    from lithtech import dtx
    tex = dtx.read_file("GL0007.dtx")     # or dtx.read(data)
    data = dtx.write(tex)                  # bytes, identical to the input
    w, h, rgba = tex.decode(mip=0)        # RGBA8, top row first

    new = dtx.from_rgba(256, 256, rgba, bpp=dtx.BPP_S3TC_DXT5, mipmaps=4)

    # PS2 textures (BPP_32P): palette alpha as the PS2 draws it
    w, h, rgba = tex.decode(0, ps2_alpha=True)
    ps2 = dtx.from_rgba(128, 128, rgba, bpp=dtx.BPP_32P)   # quantised, 3 mips

See plugins/blender/docs/DTX.md for the layout and the evidence for each
field. Standard library only, so it runs inside Blender's Python; numpy is
used for decoding when it is importable (Blender bundles it), and the pure
Python path gives the same bytes.

Conventions:
  * Rows are stored top row first, as the engine uploads them; decode()
    returns them that way. Blender images are bottom row first: use
    flip_rows().
  * 32-bit pixels and palette entries are the engine's PValue, a little
    endian 0xAARRGGBB, so the bytes are B, G, R, A.
  * Every header and section byte is kept, padding and garbage included
    (many shipped headers have runtime memory in the command string), so
    write() gives back the input exactly.

BPP_32P is the PS2's format (8-bit indices into a 256-entry PALLETE32
section): DEdit's "Convert to 32P" made every one, PS2 or PC, storing
alpha halved because the PS2's GS takes 0x80 as opaque. decode() gives
the stored alpha by default, which is what the PC engine draws;
ps2_alpha=True reads it as the PS2 draws it (ps2_palette_alpha()).

Version handling is explicit: read() and write() dispatch on the header's
version through _READERS/_WRITERS. -5 is every shipped file from NOLF to
Tron 2.0. -2 (LithTech 1.0, Blood 2) and -3 (LithTech 1.5, KISS Psycho
Circus) are 8-bit palettised with optional 4-bit alpha masks, laid out as
the engines' own loaders read them (DTX.md, "Versions -2 and -3"; LithRez's
core/src/dtx.cpp reads them the same way): read, decoded and written back
byte for byte, but not encoded (from_rgba makes -5 files).
"""

import struct
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

try:  # Blender has numpy; the plain path is the reference.
    import numpy as _np
except ImportError:  # pragma: no cover
    _np = None


# -- Constants (talon src/LT2/tools/shared/engine/dtxmgr.h, pixelformat.h) --

LT_RESTYPE_DTX = 0
CURRENT_DTX_VERSION = -5
DTX_COMMANDSTRING_LEN = 128
MAX_DTX_SIZE = 1024
MAX_DTX_MIPMAPS = 8

# BPPIdent, stored in m_Extra[2]; 0 there means BPP_32 (GetBPPIdent), so
# BPP_8P cannot be stored.
BPP_8P = 0
BPP_8 = 1
BPP_16 = 2
BPP_32 = 3
BPP_S3TC_DXT1 = 4
BPP_S3TC_DXT3 = 5
BPP_S3TC_DXT5 = 6
BPP_32P = 7          # 8-bit indices into a PALLETE32 section of 256 BGRA

BPP_NAMES = {
    BPP_8P: "8P", BPP_8: "8", BPP_16: "16", BPP_32: "32",
    BPP_S3TC_DXT1: "DXT1", BPP_S3TC_DXT3: "DXT3", BPP_S3TC_DXT5: "DXT5",
    BPP_32P: "32P",
}

# m_IFlags.
DTX_FULLBRITE = 1 << 0
DTX_PREFER16BIT = 1 << 1
DTX_MIPSALLOCED = 1 << 2      # runtime only
DTX_SECTIONSFIXED = 1 << 3    # m_nSections is valid; set in every file
DTX_NOSYSCACHE = 1 << 6
DTX_PREFER4444 = 1 << 7
DTX_PREFER5551 = 1 << 8
DTX_32BITSYSCOPY = 1 << 9
DTX_CUBEMAP = 1 << 10         # faces 2-6 in a CUBEMAPDATA section
DTX_BUMPMAP = 1 << 11
DTX_LUMBUMPMAP = 1 << 12
DTX_FLAGSAVEMASK = (DTX_FULLBRITE | DTX_32BITSYSCOPY | DTX_PREFER16BIT |
                    DTX_SECTIONSFIXED | DTX_PREFER4444 | DTX_PREFER5551 |
                    DTX_CUBEMAP | DTX_BUMPMAP | DTX_LUMBUMPMAP |
                    DTX_NOSYSCACHE)

# Versions -2/-3 share bits 0 (FULLBRITE) and 3 (SECTIONSFIXED) with -5;
# bit 1 means the file carries 4-bit alpha masks, and bit 5 keeps the
# renderer off 8-bit paletted surfaces (skies). The names are ours.
DTX_OLD_ALPHAMASKS = 1 << 1
DTX_OLD_NOPALETTED = 1 << 5
# The -2/-3 renderers make palette entries 246-255 opaque in every format
# they build, whatever a mask says.
OLD_OPAQUE_FIRST = 246
OLD_VERSIONS = (-2, -3)
OLD_GAMES = {-2: "LithTech 1.0: Blood 2", -3: "LithTech 1.5: KISS Psycho Circus"}

SECTION_PALETTE32 = "PALLETE32"   # sic
SECTION_CUBEMAP = "CUBEMAPDATA"
SECTION_LIGHTDEFS = "LIGHTDEFS"   # texture lights, read by Processor

_HEADER = struct.Struct("<Ii4H2i12s128s")       # 164 bytes (-5, -3)
_HEADER_V2 = struct.Struct("<Ii4H2i12s")         # 36 bytes (-2: no command string)
_SECTION = struct.Struct("<15s10s3sI")           # 32 bytes (3 pad)
HEADER_SIZE = _HEADER.size
SECTION_HEADER_SIZE = _SECTION.size

_PIXEL_BYTES = {BPP_8P: 1, BPP_8: 1, BPP_16: 2, BPP_32: 4, BPP_32P: 1}


def is_compressed(bpp):
    return bpp in (BPP_S3TC_DXT1, BPP_S3TC_DXT3, BPP_S3TC_DXT5)


def image_size(bpp, width, height):
    """Bytes of one mip level, as CalcImageSize (pixelformat.cpp) has it.

    S3TC sizes are not rounded up to whole 4x4 blocks: a 2x2 DXT1 mip is 2
    bytes. Shipped files are laid out by this.
    """
    if bpp == BPP_S3TC_DXT1:
        return (width * height) >> 1
    if bpp in (BPP_S3TC_DXT3, BPP_S3TC_DXT5):
        return width * height
    return width * height * _PIXEL_BYTES[bpp]


def mip_dims(width, height, n):
    """The (width, height) of each of n mips: halved, as dtx_Alloc does."""
    out = []
    for _ in range(n):
        out.append((width, height))
        width >>= 1
        height >>= 1
    return out


def max_mips_for_size(size):
    """DEdit's MaxMipsForSize (TextureDlg.cpp): mips down to 8 pixels."""
    n = 0
    while True:
        size //= 2
        n += 1
        if size < 8:
            return n


def _cstr(b):
    return b.split(b"\0", 1)[0].decode("latin-1")


def _pad_str(s, n, keep=b""):
    """s NUL-terminated in n bytes; the bytes after the NUL come from keep
    (the old raw field) so that renaming nothing changes nothing."""
    b = s.encode("latin-1")
    if len(b) >= n:
        raise ValueError(f"{s!r} does not fit in {n} bytes")
    tail = keep[len(b) + 1:n] if len(keep) == n else b""
    return (b + b"\0" + tail).ljust(n, b"\0")


@dataclass
class Section:
    """A DtxSection: a type, a name and data. type_raw and name_raw are the
    stored fields with whatever followed the NUL; pad is the 3 bytes of
    struct padding before m_DataLen."""
    type_raw: bytes = b"\0" * 15
    name_raw: bytes = b"\0" * 10
    pad: bytes = b"\0\0\0"
    data: bytes = b""

    @property
    def type(self):
        return _cstr(self.type_raw)

    @type.setter
    def type(self, s):
        self.type_raw = _pad_str(s, 15, self.type_raw)

    @property
    def name(self):
        return _cstr(self.name_raw)

    @name.setter
    def name(self, s):
        self.name_raw = _pad_str(s, 10, self.name_raw)

    @classmethod
    def make(cls, type_, data, name=""):
        s = cls(data=bytes(data))
        s.type = type_
        s.name = name
        return s


@dataclass
class Texture:
    """A .dtx file. mips holds the base image's levels (largest first);
    faces 2-6 of a cube map stay in their CUBEMAPDATA section (see
    face_mips()).

    The header fields are the stored ones; mip_count and section_count are
    written as stored, and write() checks them against the lists.
    """
    res_type: int = LT_RESTYPE_DTX
    version: int = CURRENT_DTX_VERSION
    width: int = 0
    height: int = 0
    mip_count: int = 0
    section_count: int = 0
    iflags: int = DTX_SECTIONSFIXED
    user_flags: int = 0
    extra: bytearray = field(default_factory=lambda: bytearray(12))
    command_raw: bytes = b"\0" * DTX_COMMANDSTRING_LEN
    mips: List[bytes] = field(default_factory=list)
    sections: List[Section] = field(default_factory=list)
    trailing: bytes = b""      # after the sections; empty in every shipped file
    # The file ends inside the pixel data (two Tron 2.0 prototype textures
    # lack their 2x32 DXT5 level); the short levels are kept as found.
    truncated: bool = False
    # Versions -2/-3 only: the 8-byte palette id (the engine's palette cache
    # key), the 256 palette entries as stored ({unused, R, G, B} each), and
    # with DTX_OLD_ALPHAMASKS one mask per level (rows of w/2 bytes, the low
    # nibble the even pixel's alpha; 1 byte for a 1x1 level).
    palette_id: bytes = b""
    palette_raw: bytes = b""
    masks: List[bytes] = field(default_factory=list)

    # -- m_Extra (DtxHeader accessors) --

    @property
    def is_old(self):
        return self.version in OLD_VERSIONS

    @property
    def has_alpha_masks(self):
        return self.is_old and bool(self.iflags & DTX_OLD_ALPHAMASKS)

    @property
    def old_detail_scale(self):
        """-3: the detail texture's scale, a plain float (not -5's value
        minus 1), in header bytes 28-31 (KISS's renderer, d3d.ren
        0x10023f30)."""
        return struct.unpack_from("<f", self.extra, 4)[0]

    @property
    def old_detail_angle(self):
        return struct.unpack_from("<H", self.extra, 8)[0]

    @property
    def bpp(self):
        if self.is_old:
            return BPP_8P
        return self.extra[2] or BPP_32

    @bpp.setter
    def bpp(self, v):
        self.extra[2] = v

    @property
    def bpp_name(self):
        return BPP_NAMES.get(self.bpp, str(self.bpp))

    @property
    def texture_group(self):
        return self.extra[0]

    @texture_group.setter
    def texture_group(self, v):
        self.extra[0] = v

    @property
    def mipmaps_to_use(self):
        return self.extra[1]

    @mipmaps_to_use.setter
    def mipmaps_to_use(self, v):
        self.extra[1] = v

    @property
    def non_s3tc_mip_offset(self):
        return self.extra[3]

    @non_s3tc_mip_offset.setter
    def non_s3tc_mip_offset(self, v):
        self.extra[3] = v

    @property
    def ui_mip_offset(self):
        return self.extra[4]

    @ui_mip_offset.setter
    def ui_mip_offset(self, v):
        self.extra[4] = v

    @property
    def ui_mip_scale(self):
        return float(1 << self.extra[4])

    @property
    def priority(self):
        return self.extra[5]

    @priority.setter
    def priority(self, v):
        self.extra[5] = v

    @property
    def detail_scale(self):
        # stored as the value minus 1.0, so zeroed bytes mean 1.0
        return struct.unpack_from("<f", self.extra, 6)[0] + 1.0

    @detail_scale.setter
    def detail_scale(self, v):
        struct.pack_into("<f", self.extra, 6, v - 1.0)

    @property
    def detail_angle(self):
        return struct.unpack_from("<h", self.extra, 10)[0]

    @detail_angle.setter
    def detail_angle(self, v):
        struct.pack_into("<h", self.extra, 10, v)

    @property
    def command(self):
        if self.version == -2:
            return ""
        c = _cstr(self.command_raw)
        if self.is_old and any(ord(ch) < 0x20 or ord(ch) > 0x7e for ch in c):
            return ""     # -3: no shipped file holds text there, only leftover memory
        return c

    @command.setter
    def command(self, s):
        self.command_raw = _pad_str(s, DTX_COMMANDSTRING_LEN)

    # -- Derived --

    @property
    def is_cubemap(self):
        return not self.is_old and bool(self.iflags & DTX_CUBEMAP)

    def mip_size(self, i):
        return (max(self.width >> i, 0), max(self.height >> i, 0))

    def find_section(self, type_):
        """The first section of that type (dtx_FindSection is exact case)."""
        for s in self.sections:
            if s.type == type_:
                return s
        return None

    def palette(self, ps2_alpha=False):
        """The PALLETE32 section's 256 colors as RGBA bytes, or None;
        ps2_alpha reads their alpha as the PS2 does (ps2_palette_alpha)."""
        if self.is_old:
            return old_palette_rgba(self.palette_raw)
        s = self.find_section(SECTION_PALETTE32)
        if s is None:
            return None
        pal = bgra_to_rgba(s.data[:1024].ljust(1024, b"\0"))
        return ps2_palette_alpha(pal) if ps2_alpha else pal

    @property
    def ps2_style_palette(self):
        """BPP_32P with no palette alpha above 0x80: made by DEdit's
        "Convert to 32P" for the PS2 (every PS2 32P texture, and the nine 32P
        files that went out with the PC games too)."""
        s = self.find_section(SECTION_PALETTE32)
        return (self.bpp == BPP_32P and s is not None
                and max(s.data[3:1024:4], default=0) <= 0x80)

    def face_mips(self, face):
        """The mips of a cube map face: 0 is the base image (+X), 1-5 are
        -X, +Y, -Y, +Z, -Z from CUBEMAPDATA (dtxmgr.h, DTX_CUBEMAP)."""
        if face == 0:
            return self.mips
        if not self.is_cubemap:
            raise ValueError("not a cube map")
        s = self.find_section(SECTION_CUBEMAP)
        sizes = [image_size(self.bpp, w, h)
                 for w, h in mip_dims(self.width, self.height, self.mip_count)]
        per_face = sum(sizes)
        pos = (face - 1) * per_face
        out = []
        for n in sizes:
            out.append(s.data[pos:pos + n])
            pos += n
        return out

    def decode(self, mip=0, face=0, ps2_alpha=False):
        """(width, height, RGBA bytes, top row first) of one level.
        ps2_alpha reads BPP_32P palette alpha as the PS2 does."""
        w, h = self.mip_size(mip)
        if self.is_old:
            if face:
                raise ValueError("not a cube map")
            mask = None
            if self.has_alpha_masks:
                # missing or cut short: reads as zero (transparent)
                mask = self.masks[mip] if mip < len(self.masks) else b""
            return w, h, decode_old_pixels(w, h, self.mips[mip], self.palette_raw, mask)
        data = self.face_mips(face)[mip]
        return w, h, decode_pixels(self.bpp, w, h, data,
                                   self.palette(ps2_alpha and self.bpp == BPP_32P),
                                   self.iflags)


# -- Reading and writing --

def _read_v5(data, lenient):
    (res_type, version, w, h, nmips, nsec, iflags, uflags,
     extra, cmd) = _HEADER.unpack_from(data, 0)
    t = Texture(res_type, version, w, h, nmips, nsec, iflags, uflags,
                bytearray(extra), cmd)
    if nmips == 0 or nmips > 15:          # dtx_Create's test
        raise ValueError(f"bad mip count {nmips}")
    pos = HEADER_SIZE
    for mw, mh in mip_dims(w, h, nmips):
        n = image_size(t.bpp, mw, mh)
        if pos + n > len(data):
            # dtx_Create would fail on the stream's error status; keep what
            # there is so that the file still round-trips.
            if not lenient:
                raise ValueError("pixel data runs past the end of the file")
            t.truncated = True
        t.mips.append(bytes(data[pos:pos + n]))
        pos = min(pos + n, len(data))
    if t.truncated:
        return t
    # Without DTX_SECTIONSFIXED the engine reads no sections (older files'
    # counts were not to be trusted); no shipped file lacks it.
    if iflags & DTX_SECTIONSFIXED:
        for _ in range(nsec):
            ty, nm, pad, n = _SECTION.unpack_from(data, pos)
            pos += SECTION_HEADER_SIZE
            if pos + n > len(data):
                raise ValueError("section data runs past the end of the file")
            t.sections.append(Section(ty, nm, pad, bytes(data[pos:pos + n])))
            pos += n
    t.trailing = bytes(data[pos:])
    return t


def _write_v5(t):
    if len(t.mips) != t.mip_count:
        raise ValueError("mip_count does not match mips")
    if (t.iflags & DTX_SECTIONSFIXED) and len(t.sections) != t.section_count:
        raise ValueError("section_count does not match sections")
    out = bytearray(_HEADER.pack(
        t.res_type, t.version, t.width, t.height, t.mip_count,
        t.section_count, t.iflags, t.user_flags, bytes(t.extra),
        t.command_raw))
    for (mw, mh), m in zip(mip_dims(t.width, t.height, t.mip_count), t.mips):
        if len(m) != image_size(t.bpp, mw, mh) and not t.truncated:
            raise ValueError(f"mip {mw}x{mh} is {len(m)} bytes")
        out += m
    for s in t.sections:
        out += _SECTION.pack(s.type_raw, s.name_raw, s.pad, len(s.data))
        out += s.data
    out += t.trailing
    return bytes(out)


def old_mask_size(width, height):
    """Bytes of a -2/-3 level's alpha mask (dtx_Alloc: n/2, but 1 for 1x1)."""
    n = max(width, 0) * max(height, 0)
    return n >> 1 if n > 1 else n


def _old_header(version):
    return _HEADER_V2 if version == -2 else _HEADER


def _read_old(data, lenient):
    """-2/-3 (Blood 2's Client.exe 0x420d80, KISS's 0x1cc8b40): the header,
    an 8-byte palette id and 256 four-byte palette entries, every level's
    8-bit pixels, then (DTX_OLD_ALPHAMASKS) every level's 4-bit mask, then
    sections as in -5 (none in the shipped files)."""
    version = struct.unpack_from("<i", data, 4)[0]
    hdr = _old_header(version)
    if len(data) < hdr.size + 8 + 1024:
        raise ValueError("too short for a DTX header")
    fields = hdr.unpack_from(data, 0)
    res_type, version, w, h, nmips, nsec, iflags, uflags, extra = fields[:9]
    t = Texture(res_type, version, w, h, nmips, nsec, iflags, uflags, bytearray(extra),
                fields[9] if version == -3 else b"")
    if nmips == 0 or nmips > 15:
        raise ValueError(f"bad mip count {nmips}")
    pos = hdr.size
    t.palette_id = bytes(data[pos:pos + 8])
    t.palette_raw = bytes(data[pos + 8:pos + 8 + 1024])
    pos += 8 + 1024

    def take(n):
        nonlocal pos
        if pos + n > len(data):
            if not lenient:
                raise ValueError("pixel data runs past the end of the file")
            t.truncated = True
        b = bytes(data[pos:pos + n])
        pos = min(pos + n, len(data))
        return b

    dims = mip_dims(w, h, nmips)
    t.mips = [take(mw * mh) for mw, mh in dims]
    if iflags & DTX_OLD_ALPHAMASKS:
        t.masks = [take(old_mask_size(mw, mh)) for mw, mh in dims]
    if t.truncated:
        return t
    if iflags & DTX_SECTIONSFIXED:
        for _ in range(nsec):
            ty, nm, pad, n = _SECTION.unpack_from(data, pos)
            pos += SECTION_HEADER_SIZE
            if pos + n > len(data):
                raise ValueError("section data runs past the end of the file")
            t.sections.append(Section(ty, nm, pad, bytes(data[pos:pos + n])))
            pos += n
    t.trailing = bytes(data[pos:])
    return t


def _write_old(t):
    if len(t.mips) != t.mip_count:
        raise ValueError("mip_count does not match mips")
    if (t.iflags & DTX_SECTIONSFIXED) and len(t.sections) != t.section_count:
        raise ValueError("section_count does not match sections")
    if len(t.palette_id) != 8 or len(t.palette_raw) != 1024:
        raise ValueError("a -2/-3 texture needs an 8-byte palette id and 1024 palette bytes")
    head = (t.res_type, t.version, t.width, t.height, t.mip_count, t.section_count,
            t.iflags, t.user_flags, bytes(t.extra))
    if t.version == -3:
        head += (t.command_raw,)
    out = bytearray(_old_header(t.version).pack(*head))
    out += t.palette_id + t.palette_raw
    dims = mip_dims(t.width, t.height, t.mip_count)
    for (mw, mh), m in zip(dims, t.mips):
        if len(m) != mw * mh and not t.truncated:
            raise ValueError(f"mip {mw}x{mh} is {len(m)} bytes")
        out += m
    if t.iflags & DTX_OLD_ALPHAMASKS:
        if len(t.masks) != t.mip_count and not t.truncated:
            raise ValueError("one alpha mask per level is needed")
        for (mw, mh), m in zip(dims, t.masks):
            if len(m) != old_mask_size(mw, mh) and not t.truncated:
                raise ValueError(f"mask {mw}x{mh} is {len(m)} bytes")
            out += m
    for s in t.sections:
        out += _SECTION.pack(s.type_raw, s.name_raw, s.pad, len(s.data))
        out += s.data
    out += t.trailing
    return bytes(out)


_READERS = {-5: _read_v5, -3: _read_old, -2: _read_old}
_WRITERS = {-5: _write_v5, -3: _write_old, -2: _write_old}


def read(data, lenient=True):
    """A Texture from a file's bytes. lenient keeps a file that ends inside
    its pixel data (Texture.truncated) instead of raising."""
    if len(data) < 8:
        raise ValueError("too short for a DTX header")
    res_type, version = struct.unpack_from("<Ii", data, 0)
    if version not in OLD_VERSIONS and len(data) < HEADER_SIZE:
        raise ValueError("too short for a DTX header")
    if res_type != LT_RESTYPE_DTX:
        raise ValueError(f"not a DTX (resource type {res_type})")
    reader = _READERS.get(version)
    if reader is None:
        raise ValueError(f"unsupported DTX version {version}")
    return reader(data, lenient)


def read_file(path, lenient=True):
    with open(path, "rb") as f:
        return read(f.read(), lenient)


def write(t):
    writer = _WRITERS.get(t.version)
    if writer is None:
        raise ValueError(f"unsupported DTX version {t.version}")
    return writer(t)


def write_file(t, path):
    with open(path, "wb") as f:
        f.write(write(t))


def sync_counts(t):
    """Set mip_count and section_count from the lists (as dtx_Save counts
    the sections) after editing them."""
    t.mip_count = len(t.mips)
    t.section_count = len(t.sections)
    t.iflags |= DTX_SECTIONSFIXED


# -- Pixel helpers --

def old_palette_rgba(palette_raw):
    """A -2/-3 palette ({unused, R, G, B} entries: d3d.ren builds colours
    from bytes 1-3) as 256 opaque RGBA entries."""
    p = bytes(palette_raw[:1024]).ljust(1024, b"\0")
    out = bytearray(1024)
    out[0::4] = p[1::4]
    out[1::4] = p[2::4]
    out[2::4] = p[3::4]
    out[3::4] = b"\xff" * 256
    return bytes(out)


def decode_old_pixels(width, height, data, palette_raw, mask=None):
    """RGBA from one -2/-3 level. Without a mask every pixel is opaque; with
    one (bytes, possibly short or empty: the rest reads as 0), alpha is the
    pixel's nibble * 17, except palette entries 246-255, always opaque."""
    pal = old_palette_rgba(palette_raw)
    n = max(width, 0) * max(height, 0)
    data = bytes(data[:n]).ljust(n, b"\0")
    out = bytearray(n * 4)
    if _np is not None and n:
        p = _np.frombuffer(pal, _np.uint8).reshape(256, 4)
        idx = _np.frombuffer(data, _np.uint8)
        px = p[idx].copy()
        if mask is not None:
            stride = width >> 1
            m = _np.zeros(height * stride, _np.uint8)
            mb = _np.frombuffer(bytes(mask[:height * stride]), _np.uint8)
            m[:len(mb)] = mb
            m = m.reshape(height, stride) if stride else m.reshape(height, 0)
            a = _np.zeros((height, width), _np.uint8)
            a[:, 0:stride * 2:2] = (m & 15) * 17
            a[:, 1:stride * 2:2] = (m >> 4) * 17
            a = a.reshape(-1)
            px[:, 3] = _np.where(idx >= OLD_OPAQUE_FIRST, 255, a)
        return px.tobytes()
    stride = width >> 1
    for i in range(n):
        y, x = divmod(i, width) if width else (0, 0)
        c = data[i]
        o = 4 * i
        out[o:o + 3] = pal[4 * c:4 * c + 3]
        if mask is None or c >= OLD_OPAQUE_FIRST:
            out[o + 3] = 255
        elif x < stride * 2:
            k = y * stride + (x >> 1)
            v = mask[k] if k < len(mask) else 0
            out[o + 3] = ((v >> (4 * (x & 1))) & 15) * 17
        else:
            out[o + 3] = 0
    return bytes(out)

def bgra_to_rgba(b):
    b = bytearray(b)
    b[0::4], b[2::4] = b[2::4], b[0::4]
    return bytes(b)


rgba_to_bgra = bgra_to_rgba


def ps2_palette_alpha(palette):
    """A 256-entry RGBA (or BGRA) palette with its alpha read as the PS2
    draws it, the inverse of DEdit's halving (LTTexture's
    Convert32BitTo8BitPalletized stores a/2; the GS takes 0x80 as opaque):
    0x7f and up is 255, below that a * 255 / 127 rounded. A palette whose
    alpha is all zero (made from a texture with no alpha) is opaque."""
    pal = bytearray(palette)
    alphas = pal[3::4]
    if not any(alphas):
        pal[3::4] = b"\xff" * len(alphas)
    else:
        pal[3::4] = bytes(255 if a >= 0x7f else (a * 255 + 63) // 127 for a in alphas)
    return bytes(pal)


def ps2_store_alpha(a):
    """An alpha (0-255, as drawn) as DEdit stores it in a PS2 palette: a/2."""
    return a >> 1


def flip_rows(rgba, width, height):
    """Top row first <-> bottom row first (Blender's order)."""
    stride = width * 4
    return b"".join(rgba[y * stride:(y + 1) * stride]
                    for y in range(height - 1, -1, -1))


# The engine's 5- and 6-bit to 8-bit scales: j*255/max, truncated
# (FormatMgr::InitScaleTables).
_S5 = bytes((j * 255) // 31 for j in range(32))
_S6 = bytes((j * 255) // 63 for j in range(64))
_S4 = bytes(j * 17 for j in range(16))     # g_FullAlphaValues


def _rgb565(v):
    return _S5[v >> 11], _S6[(v >> 5) & 63], _S5[v & 31]


def decode_pixels(bpp, width, height, data, palette=None, iflags=0):
    """One level's pixels as RGBA bytes, top row first.

    palette is RGBA (Texture.palette()), needed for BPP_32P. BPP_16 has no
    shipped file to say its layout: it is read as RGB565 unless iflags
    prefers 4444 or 5551.
    """
    if width <= 0 or height <= 0:
        return b""
    if bpp == BPP_32:
        return bgra_to_rgba(data[:width * height * 4])
    if bpp == BPP_32P:
        if palette is None:
            raise ValueError("BPP_32P texture without a PALLETE32 section")
        if _np is not None:
            pal = _np.frombuffer(palette, _np.uint8).reshape(256, 4)
            idx = _np.frombuffer(data, _np.uint8, width * height)
            return pal[idx].tobytes()
        return b"".join(palette[i * 4:i * 4 + 4] for i in data[:width * height])
    if bpp == BPP_16:
        return _decode_16(width, height, data, iflags)
    if is_compressed(bpp):
        if _np is not None:
            return _decode_dxt_np(bpp, width, height, data)
        return _decode_dxt(bpp, width, height, data)
    raise NotImplementedError(f"decoding {BPP_NAMES.get(bpp, bpp)}")


def _decode_16(width, height, data, iflags):
    out = bytearray(width * height * 4)
    vals = struct.unpack_from(f"<{width * height}H", data)
    for i, v in enumerate(vals):
        if iflags & DTX_PREFER4444:
            px = (_S4[(v >> 8) & 15], _S4[(v >> 4) & 15], _S4[v & 15],
                  _S4[v >> 12])
        elif iflags & DTX_PREFER5551:
            px = (_S5[(v >> 10) & 31], _S5[(v >> 5) & 31], _S5[v & 31],
                  255 if v & 0x8000 else 0)
        else:
            px = _rgb565(v) + (255,)
        out[i * 4:i * 4 + 4] = bytes(px)
    return bytes(out)


def _block_bytes(bpp):
    return 8 if bpp == BPP_S3TC_DXT1 else 16


def _color_table(c0, c1, four):
    r0, g0, b0 = _rgb565(c0)
    r1, g1, b1 = _rgb565(c1)
    if four:
        return [(r0, g0, b0, 255), (r1, g1, b1, 255),
                ((2 * r0 + r1) // 3, (2 * g0 + g1) // 3, (2 * b0 + b1) // 3, 255),
                ((r0 + 2 * r1) // 3, (g0 + 2 * g1) // 3, (b0 + 2 * b1) // 3, 255)]
    return [(r0, g0, b0, 255), (r1, g1, b1, 255),
            ((r0 + r1) >> 1, (g0 + g1) >> 1, (b0 + b1) >> 1, 255),
            (0, 0, 0, 0)]


def _alpha_table(a0, a1):
    if a0 > a1:
        return [a0, a1] + [((7 - i) * a0 + i * a1) // 7 for i in range(1, 7)]
    return [a0, a1] + [((5 - i) * a0 + i * a1) // 5 for i in range(1, 5)] + [0, 255]


def _decode_dxt(bpp, width, height, data):
    """S3TC to RGBA, the engine's integer math (ConvertDXTGeneric).

    DXT1 picks 4 colors when c0 > c1, else 3 and transparent black; DXT3 and
    DXT5 always use 4 colors, as the hardware the game drew with does (the
    engine's software path tests c0 > c1 for those too). Levels smaller
    than a block are stored short (image_size); missing bytes read as 0.
    """
    bw, bh = (width + 3) // 4, (height + 3) // 4
    bb = _block_bytes(bpp)
    need = bw * bh * bb
    if len(data) < need:
        data = bytes(data) + b"\0" * (need - len(data))
    out = bytearray(width * height * 4)
    stride = width * 4
    pos = 0
    for by in range(bh):
        for bx in range(bw):
            if bpp == BPP_S3TC_DXT1:
                alpha = None
                cpos = pos
            else:
                cpos = pos + 8
                if bpp == BPP_S3TC_DXT3:
                    a = int.from_bytes(data[pos:pos + 8], "little")
                    alpha = [_S4[(a >> (4 * i)) & 15] for i in range(16)]
                else:
                    tab = _alpha_table(data[pos], data[pos + 1])
                    a = int.from_bytes(data[pos + 2:pos + 8], "little")
                    alpha = [tab[(a >> (3 * i)) & 7] for i in range(16)]
            c0, c1, bits = struct.unpack_from("<HHI", data, cpos)
            colors = _color_table(c0, c1, bpp != BPP_S3TC_DXT1 or c0 > c1)
            for py in range(4):
                y = by * 4 + py
                if y >= height:
                    break
                row = y * stride
                for px in range(4):
                    x = bx * 4 + px
                    if x >= width:
                        break
                    i = py * 4 + px
                    c = colors[(bits >> (2 * i)) & 3]
                    o = row + x * 4
                    if alpha is None:
                        out[o:o + 4] = bytes(c)
                    else:
                        out[o:o + 4] = bytes((c[0], c[1], c[2], alpha[i]))
            pos += bb
    return bytes(out)


def _decode_dxt_np(bpp, width, height, data):
    np = _np
    bw, bh = (width + 3) // 4, (height + 3) // 4
    bb = _block_bytes(bpp)
    raw = bytes(data).ljust(bw * bh * bb, b"\0")
    blk = np.frombuffer(raw, np.uint8).reshape(bw * bh, bb)
    col = blk[:, bb - 8:]
    c0 = col[:, 0].astype(np.int32) | (col[:, 1].astype(np.int32) << 8)
    c1 = col[:, 2].astype(np.int32) | (col[:, 3].astype(np.int32) << 8)
    bits = col[:, 4:8].copy().view("<u4")[:, 0].astype(np.int64)
    s5 = np.frombuffer(_S5, np.uint8).astype(np.int32)
    s6 = np.frombuffer(_S6, np.uint8).astype(np.int32)

    def rgb(c):
        return np.stack([s5[c >> 11], s6[(c >> 5) & 63], s5[c & 31]], 1)

    e0, e1 = rgb(c0), rgb(c1)
    n = len(blk)
    pal = np.zeros((n, 4, 4), np.int32)
    pal[:, 0, :3], pal[:, 1, :3] = e0, e1
    pal[:, :2, 3] = 255
    four = (c0 > c1) if bpp == BPP_S3TC_DXT1 else np.ones(n, bool)
    pal[:, 2, :3] = np.where(four[:, None], (2 * e0 + e1) // 3, (e0 + e1) >> 1)
    pal[:, 3, :3] = np.where(four[:, None], (e0 + 2 * e1) // 3, 0)
    pal[:, 2, 3] = 255
    pal[:, 3, 3] = np.where(four, 255, 0)
    shifts = np.arange(16) * 2
    idx = (bits[:, None] >> shifts) & 3                          # n x 16
    px = pal[np.arange(n)[:, None], idx]                         # n x 16 x 4
    if bpp == BPP_S3TC_DXT3:
        a = blk[:, :8].copy().view("<u8")[:, 0]
        av = (a[:, None] >> (np.arange(16, dtype=np.uint64) * np.uint64(4))) & np.uint64(15)
        px[:, :, 3] = av.astype(np.int32) * 17
    elif bpp == BPP_S3TC_DXT5:
        a0 = blk[:, 0].astype(np.int32)
        a1 = blk[:, 1].astype(np.int32)
        tab = np.zeros((n, 8), np.int32)
        tab[:, 0], tab[:, 1] = a0, a1
        gt = a0 > a1
        for i in range(1, 7):
            tab[:, i + 1] = ((7 - i) * a0 + i * a1) // 7
        for i in range(1, 5):
            tab[:, i + 1] = np.where(gt, tab[:, i + 1], ((5 - i) * a0 + i * a1) // 5)
        tab[:, 6] = np.where(gt, tab[:, 6], 0)
        tab[:, 7] = np.where(gt, tab[:, 7], 255)
        ab = np.zeros(n, np.uint64)
        for k in range(6):
            ab |= blk[:, 2 + k].astype(np.uint64) << np.uint64(8 * k)
        ai = (ab[:, None] >> (np.arange(16, dtype=np.uint64) * np.uint64(3))) & np.uint64(7)
        px[:, :, 3] = tab[np.arange(n)[:, None], ai.astype(np.int64)]
    img = px.reshape(bh, bw, 4, 4, 4).transpose(0, 2, 1, 3, 4)
    img = img.reshape(bh * 4, bw * 4, 4)[:height, :width]
    return img.astype(np.uint8).tobytes()


# -- Encoding --

def build_mipmaps(width, height, rgba, count):
    """count levels from RGBA, each the 2x2 box average of the one before,
    truncated, as dtx_BuildMipmaps does (on BGRA; the order doesn't
    matter to an average per byte)."""
    levels = [bytes(rgba)]
    w, h = width, height
    for _ in range(1, count):
        src = levels[-1]
        nw, nh = w >> 1, h >> 1
        out = bytearray(nw * nh * 4)
        s = w * 4
        for y in range(nh):
            r1 = 2 * y * s
            r2 = r1 + s
            o = y * nw * 4
            for x in range(nw):
                p = x * 8
                for c in range(4):
                    out[o + c] = (src[r1 + p + c] + src[r1 + p + 4 + c] +
                                  src[r2 + p + c] + src[r2 + p + 4 + c]) >> 2
                o += 4
        levels.append(bytes(out))
        w, h = nw, nh
    return levels


def _to565(r, g, b):
    return ((r * 31 + 127) // 255) << 11 | ((g * 63 + 127) // 255) << 5 | ((b * 31 + 127) // 255)


def _encode_color_block(px, allow_3color):
    """px: 16 (r, g, b, a). Endpoints from the extremes along the bounding
    box diagonal, indices by nearest color (the engine's tables)."""
    trans = allow_3color and any(p[3] < 128 for p in px)
    pts = [p for p in px if not (trans and p[3] < 128)] or px
    lo = [min(p[i] for p in pts) for i in range(3)]
    hi = [max(p[i] for p in pts) for i in range(3)]
    axis = [hi[i] - lo[i] for i in range(3)]
    proj = [sum(p[i] * axis[i] for i in range(3)) for p in pts]
    pmax = pts[proj.index(max(proj))]
    pmin = pts[proj.index(min(proj))]
    c0, c1 = _to565(*pmax[:3]), _to565(*pmin[:3])
    if trans:
        if c0 > c1:
            c0, c1 = c1, c0
        table = _color_table(c0, c1, False)
        usable = 3
    else:
        if c0 < c1:
            c0, c1 = c1, c0
        if c0 == c1:
            return struct.pack("<HHI", c0, c1, 0)
        table = _color_table(c0, c1, True)
        usable = 4
    bits = 0
    for i, p in enumerate(px):
        if trans and p[3] < 128:
            k = 3
        else:
            k = min(range(usable), key=lambda j: (table[j][0] - p[0]) ** 2 +
                    (table[j][1] - p[1]) ** 2 + (table[j][2] - p[2]) ** 2)
        bits |= k << (2 * i)
    return struct.pack("<HHI", c0, c1, bits)


def _encode_dxt5_alpha(alphas):
    a0, a1 = max(alphas), min(alphas)
    if a0 == a1:
        return bytes((a0, a1)) + b"\0" * 6
    tab = _alpha_table(a0, a1)
    bits = 0
    for i, a in enumerate(alphas):
        k = min(range(8), key=lambda j: abs(tab[j] - a))
        bits |= k << (3 * i)
    return bytes((a0, a1)) + bits.to_bytes(6, "little")


def encode_pixels(bpp, width, height, rgba):
    """RGBA (top row first) in a stored format. Pure Python: a simple S3TC
    compressor, not the one the original tools used (so re-encoding a
    shipped DXT texture will not give its bytes back)."""
    if bpp == BPP_32:
        return rgba_to_bgra(rgba)
    if bpp == BPP_16:
        out = bytearray()
        for i in range(0, width * height * 4, 4):
            out += struct.pack("<H", _to565(rgba[i], rgba[i + 1], rgba[i + 2]))
        return bytes(out)
    if not is_compressed(bpp):
        raise NotImplementedError(f"encoding {BPP_NAMES.get(bpp, bpp)}")
    out = bytearray()
    stride = width * 4
    nbx, nby = max(width // 4, 1), max(height // 4, 1)
    for by in range(nby):
        for bx in range(nbx):
            px = []
            for py in range(4):
                y = min(by * 4 + py, height - 1)
                for pxx in range(4):
                    x = min(bx * 4 + pxx, width - 1)
                    o = y * stride + x * 4
                    px.append(tuple(rgba[o:o + 4]))
            if bpp == BPP_S3TC_DXT3:
                a = 0
                for i, p in enumerate(px):
                    a |= ((p[3] * 15 + 127) // 255) << (4 * i)
                out += a.to_bytes(8, "little")
            elif bpp == BPP_S3TC_DXT5:
                out += _encode_dxt5_alpha([p[3] for p in px])
            out += _encode_color_block(px, bpp == BPP_S3TC_DXT1)
    # Sub-block levels are stored short (image_size).
    return bytes(out[:image_size(bpp, width, height)])


def quantize(rgba, colors=256):
    """(palette, indices) for RGBA pixels: palette is up to `colors` RGBA
    entries (bytes, 4 each), indices one byte per pixel. Exact when there
    are no more distinct colors than that (most used first, as DEdit
    orders them); otherwise a median cut in RGBA, each pixel mapped to its
    nearest entry. (DEdit took the 256 most used colors and, through a bug
    in its NearestColor, gave every other pixel the last entry.)"""
    n = len(rgba) // 4
    if _np is not None:
        px = _np.frombuffer(rgba, _np.uint8, n * 4).reshape(n, 4)
        key = px.copy().view("<u4")[:, 0]
        uniq, first, inv, counts = _np.unique(key, return_index=True,
                                              return_inverse=True,
                                              return_counts=True)
        if len(uniq) <= colors:
            order = _np.lexsort((first, -counts))       # most used, then first seen
            rank = _np.empty(len(uniq), _np.int64)
            rank[order] = _np.arange(len(uniq))
            pal = px[first[order]]
            return pal.tobytes(), rank[inv.reshape(-1)].astype(_np.uint8).tobytes()
        boxes = [_np.arange(n)]
        while len(boxes) < colors:
            # split the box with the widest channel range, at its median
            best, bi, bc = -1, 0, 0
            for i, b in enumerate(boxes):
                if len(b) < 2:
                    continue
                v = px[b]
                rng = v.max(0).astype(int) - v.min(0).astype(int)
                c = int(rng.argmax())
                if rng[c] > best:
                    best, bi, bc = int(rng[c]), i, c
            if best <= 0:
                break
            b = boxes.pop(bi)
            o = b[_np.argsort(px[b, bc], kind="stable")]
            boxes += [o[:len(o) // 2], o[len(o) // 2:]]
        pal = _np.array([px[b].mean(0) + 0.5 for b in boxes]).astype(_np.uint8)
        pf = pal.astype(_np.int32)
        idx = _np.empty(n, _np.uint8)
        for s in range(0, n, 65536):
            d = px[s:s + 65536].astype(_np.int32)[:, None, :] - pf[None, :, :]
            idx[s:s + 65536] = (d * d).sum(2).argmin(1)
        return pal.tobytes(), idx.tobytes()
    # The plain path: same results, slowly.
    pix = [bytes(rgba[i * 4:i * 4 + 4]) for i in range(n)]
    counts, first = {}, {}
    for i, p in enumerate(pix):
        counts[p] = counts.get(p, 0) + 1
        first.setdefault(p, i)
    if len(counts) <= colors:
        order = sorted(counts, key=lambda p: (-counts[p], first[p]))
        rank = {p: i for i, p in enumerate(order)}
        return b"".join(order), bytes(rank[p] for p in pix)
    boxes = [list(range(n))]
    while len(boxes) < colors:
        best, bi, bc = -1, 0, 0
        for i, b in enumerate(boxes):
            if len(b) < 2:
                continue
            for c in range(4):
                vals = [pix[k][c] for k in b]
                r = max(vals) - min(vals)
                if r > best:
                    best, bi, bc = r, i, c
        if best <= 0:
            break
        b = sorted(boxes.pop(bi), key=lambda k: pix[k][bc])
        boxes += [b[:len(b) // 2], b[len(b) // 2:]]
    pal = [bytes(int(sum(pix[k][c] for k in b) / len(b) + 0.5) for c in range(4)) for b in boxes]
    cache = {}

    def near(p):
        if p not in cache:
            cache[p] = min(range(len(pal)), key=lambda j: sum(
                (pal[j][c] - p[c]) ** 2 for c in range(4)))
        return cache[p]
    return b"".join(pal), bytes(near(p) for p in pix)


def nearest_indices(palette, rgba):
    """Each pixel's nearest entry (squared RGBA distance) in an RGBA palette."""
    n = len(rgba) // 4
    if _np is not None:
        pf = _np.frombuffer(palette, _np.uint8).reshape(-1, 4).astype(_np.int32)
        px = _np.frombuffer(rgba, _np.uint8, n * 4).reshape(n, 4).astype(_np.int32)
        idx = _np.empty(n, _np.uint8)
        for s in range(0, n, 65536):
            d = px[s:s + 65536, None, :] - pf[None, :, :]
            idx[s:s + 65536] = (d * d).sum(2).argmin(1)
        return idx.tobytes()
    pal = [palette[j * 4:j * 4 + 4] for j in range(len(palette) // 4)]
    return bytes(min(range(len(pal)), key=lambda j: sum(
        (pal[j][c] - rgba[i * 4 + c]) ** 2 for c in range(4))) for i in range(n))


def encode_32p(width, height, rgba, mipmaps, ps2_alpha=True, palette=None,
               stored_palette=None):
    """(PALLETE32 data, [mip index levels]) for RGBA, top row first.

    The palette comes from quantize() of the base level, unless one is given
    (RGBA, as drawn): then every level is mapped onto it. Smaller levels are
    box filtered (build_mipmaps) and mapped onto the same palette. With
    ps2_alpha the stored alpha is halved as DEdit stores it for the PS2
    (ps2_store_alpha); else it is stored as given. stored_palette (the
    PALLETE32 bytes the given palette was read from) is written as it is,
    so that a texture mapped back onto its own palette keeps it exactly."""
    levels = build_mipmaps(width, height, rgba, mipmaps)
    if palette is None:
        palette, base = quantize(levels[0])
    else:
        base = nearest_indices(palette, levels[0])
    palette = bytes(palette).ljust(1024, b"\0")
    mips = [base] + [nearest_indices(palette, lv) for lv in levels[1:]]
    if stored_palette is not None:
        return bytes(stored_palette), mips
    store = bytearray(rgba_to_bgra(palette))
    if ps2_alpha:
        store[3::4] = bytes(ps2_store_alpha(a) for a in store[3::4])
    return bytes(store), mips


def from_rgba(width, height, rgba, bpp=BPP_S3TC_DXT5, mipmaps=None,
              iflags=DTX_SECTIONSFIXED, user_flags=0, command="",
              texture_group=0, priority=0, detail_scale=1.0, detail_angle=0,
              ps2_alpha=True, palette=None, stored_palette=None):
    """A new texture from RGBA (top row first). mipmaps defaults to DEdit's
    largest count (down to 8 pixels, at most 8). The header is laid out as
    the tools' dtx_Alloc + dtx_Save would: unused bytes zero.

    BPP_32P is written as DEdit's "Convert to 32P" writes PS2 textures:
    three mips by default (CreateMipMaps(3)), only the FULLBRITE,
    SECTIONSFIXED and 32BITSYSCOPY flags, and a PALLETE32 section (see
    encode_32p for ps2_alpha and palette)."""
    if mipmaps is None:
        if bpp == BPP_32P:      # three, as long as the levels last
            mipmaps = min(3, min(width, height).bit_length())
        else:
            mipmaps = min(MAX_DTX_MIPMAPS, max_mips_for_size(width),
                          max_mips_for_size(height))
    if bpp == BPP_32P:
        iflags &= DTX_FULLBRITE | DTX_SECTIONSFIXED | DTX_32BITSYSCOPY
    t = Texture(width=width, height=height, iflags=iflags,
                user_flags=user_flags)
    t.bpp = bpp
    t.mipmaps_to_use = mipmaps
    t.texture_group = texture_group
    t.priority = priority
    t.detail_scale = detail_scale
    t.detail_angle = detail_angle
    t.command = command
    if bpp == BPP_32P:
        pal, t.mips = encode_32p(width, height, rgba, mipmaps, ps2_alpha, palette,
                                 stored_palette)
        t.sections.append(Section.make(SECTION_PALETTE32, pal))
    else:
        for (w, h), level in zip(mip_dims(width, height, mipmaps),
                                 build_mipmaps(width, height, rgba, mipmaps)):
            t.mips.append(encode_pixels(bpp, w, h, level))
    sync_counts(t)
    return t
