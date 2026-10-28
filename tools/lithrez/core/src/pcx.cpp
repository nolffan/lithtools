#include <lithtech/pcx.hpp>

#include <algorithm>
#include <cstring>

namespace lithtech::pcx {

namespace {

uint16_t u16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }

// The palette a version 0 or 3 file implies: the 16 EGA colours.
const uint8_t kEgaDefault[48] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0x00, 0xaa, 0xaa,
    0xaa, 0x00, 0x00, 0xaa, 0x00, 0xaa, 0xaa, 0x55, 0x00, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0x55, 0xff, 0x55, 0xff, 0x55, 0x55, 0xff, 0xff,
    0xff, 0x55, 0x55, 0xff, 0x55, 0xff, 0xff, 0xff, 0x55, 0xff, 0xff, 0xff,
};

Header parse_header(const uint8_t* d) {
    Header h;
    h.manufacturer = d[0];
    h.version = d[1];
    h.encoding = d[2];
    h.bits_per_pixel = d[3];
    h.xmin = u16(d + 4), h.ymin = u16(d + 6), h.xmax = u16(d + 8), h.ymax = u16(d + 10);
    h.hdpi = u16(d + 12), h.vdpi = u16(d + 14);
    std::memcpy(h.ega_palette, d + 16, 48);
    h.planes = d[65];
    h.bytes_per_line = u16(d + 66);
    h.palette_info = u16(d + 68);
    return h;
}

bool supported(const Header& h) {
    switch (h.bits_per_pixel) {
    case 1: return h.planes >= 1 && h.planes <= 4;
    case 2: case 4: return h.planes == 1;
    case 8: return h.planes == 1 || h.planes == 3 || h.planes == 4;
    default: return false;
    }
}

}  // namespace

bool looks_like(const uint8_t* d, std::size_t size) {
    if (size < 128 || d[0] != 10) return false;
    if (d[1] != 0 && d[1] != 2 && d[1] != 3 && d[1] != 4 && d[1] != 5) return false;
    if (d[2] > 1) return false;
    Header h = parse_header(d);
    return supported(h) && h.xmax >= h.xmin && h.ymax >= h.ymin && h.bytes_per_line > 0;
}

std::string layout_name(const Header& h) {
    int bpp = h.bits_per_pixel, n = h.planes;
    if (bpp == 8 && n == 1) return "8-bit indexed";
    if (bpp == 8 && n == 3) return "24-bit RGB";
    if (bpp == 8 && n == 4) return "32-bit RGBA";
    if (bpp == 1 && n == 1) return "monochrome";
    if (bpp == 1) return std::to_string(1 << n) + "-colour EGA (" + std::to_string(n) + " planes)";
    return std::to_string(1 << bpp) + "-colour packed (" + std::to_string(bpp) + " bits/pixel)";
}

Image read(const uint8_t* d, std::size_t size) {
    if (size < 128) throw Error("too short for a PCX header");
    if (d[0] != 10) throw Error("not a PCX file (manufacturer byte " + std::to_string(d[0]) + ", not 10)");
    Image img;
    Header& h = img.header;
    h = parse_header(d);
    if (h.encoding > 1) throw Error("unknown encoding " + std::to_string(h.encoding));
    if (!supported(h))
        throw Error("unsupported layout: " + std::to_string(h.bits_per_pixel) + " bits/pixel, " +
                    std::to_string(h.planes) + " planes");
    if (h.xmax < h.xmin || h.ymax < h.ymin) throw Error("empty image window");
    img.width = h.xmax - h.xmin + 1;
    img.height = h.ymax - h.ymin + 1;
    const int bpl = h.bytes_per_line, n = h.planes, bpp = h.bits_per_pixel;
    // a line holds at most bytes_per_line * 8 / bits_per_pixel pixels
    if (img.width > bpl * 8 / bpp) throw Error("the image is wider than its scanlines");
    if (std::size_t(img.width) * img.height > (1u << 26)) throw Error("implausibly large image");

    // The palette at the end: 0x0C, then 256 RGB triples.
    std::size_t end = size;
    const uint8_t* vga = nullptr;
    if (bpp == 8 && n == 1 && size >= 128 + 769 && d[size - 769] == 12) {
        vga = d + size - 768;
        end = size - 769;
    }

    // The pixel data: one RLE stream of height scanlines, each n planes of
    // bpl bytes.
    const std::size_t line = std::size_t(bpl) * n;
    std::vector<uint8_t> raw(line * img.height);
    std::size_t o = 0, p = 128;
    if (h.encoding == 0) {
        std::size_t k = std::min(raw.size(), end > p ? end - p : 0);
        std::memcpy(raw.data(), d + p, k);
        o = k;
    } else {
        while (o < raw.size() && p < end) {
            uint8_t b = d[p++];
            if ((b & 0xc0) == 0xc0) {
                int count = b & 0x3f;
                if (p >= end) break;
                uint8_t v = d[p++];
                std::size_t k = std::min<std::size_t>(count, raw.size() - o);
                std::memset(raw.data() + o, v, k);
                o += k;
            } else {
                raw[o++] = b;
            }
        }
    }
    img.truncated = o < raw.size();

    img.rgba.assign(std::size_t(img.width) * img.height * 4, 0);
    // The colours of an indexed image.
    uint8_t pal[256 * 3] = {};
    if (!(bpp == 8 && n >= 3)) {
        if (bpp == 8) {
            img.colours = 256;
            if (vga) {
                std::memcpy(pal, vga, 768);
                img.palette = "VGA";
            } else {
                for (int i = 0; i < 256; ++i) pal[3 * i] = pal[3 * i + 1] = pal[3 * i + 2] = uint8_t(i);
                img.palette = "greyscale";
            }
        } else {
            img.colours = 1 << (bpp * n);
            bool header = h.version != 0 && h.version != 3;
            if (bpp == 1 && n == 1 && (!header || (std::all_of(h.ega_palette, h.ega_palette + 6,
                                                               [](uint8_t c) { return c == 0; })))) {
                // monochrome with no colours given: 0 black, 1 white
                pal[3] = pal[4] = pal[5] = 255;
                img.palette = "black and white";
            } else {
                std::memcpy(pal, header ? h.ega_palette : kEgaDefault, 48);
                img.palette = header ? "header" : "EGA defaults";
            }
        }
    }

    for (int y = 0; y < img.height; ++y) {
        const uint8_t* src = raw.data() + line * y;
        uint8_t* dst = img.rgba.data() + std::size_t(y) * img.width * 4;
        for (int x = 0; x < img.width; ++x, dst += 4) {
            if (bpp == 8 && n >= 3) {
                dst[0] = src[x];
                dst[1] = src[bpl + x];
                dst[2] = src[2 * bpl + x];
                dst[3] = n == 4 ? src[3 * bpl + x] : 255;
                continue;
            }
            int index;
            if (bpp == 8) {
                index = src[x];
            } else if (bpp == 1) {
                index = 0;
                for (int k = 0; k < n; ++k)
                    index |= ((src[k * bpl + (x >> 3)] >> (7 - (x & 7))) & 1) << k;
            } else {
                int per = 8 / bpp, shift = 8 - bpp * (x % per + 1);
                index = (src[x / per] >> shift) & ((1 << bpp) - 1);
            }
            dst[0] = pal[3 * index];
            dst[1] = pal[3 * index + 1];
            dst[2] = pal[3 * index + 2];
            dst[3] = 255;
        }
    }
    return img;
}

}  // namespace lithtech::pcx
