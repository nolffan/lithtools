#include <lithtech/bmp.hpp>

#include <algorithm>
#include <cstring>

namespace lithtech::bmp {

namespace {

uint16_t u16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
uint32_t u32(const uint8_t* p) { return uint32_t(p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24); }

// A value under a mask, scaled to 8 bits.
uint8_t field(uint32_t v, uint32_t mask) {
    if (!mask) return 0;
    int shift = 0;
    while (!((mask >> shift) & 1)) ++shift;
    uint32_t m = mask >> shift, x = (v & mask) >> shift;
    int bits = 0;
    while ((m >> bits) & 1) ++bits;
    if (bits >= 8) return uint8_t(x >> (bits - 8));
    // replicate the bits to fill 8 (5 bits: abcde -> abcdeabc)
    uint32_t out = 0;
    for (int got = 0; got < 8; got += bits) out = (out << bits) | x;
    int total = ((8 + bits - 1) / bits) * bits;
    return uint8_t(out >> (total - 8));
}

}  // namespace

std::string header_name(uint32_t hs) {
    switch (hs) {
    case 12: return "OS/2 1.x (12 bytes)";
    case 40: return "Windows 3 (40 bytes)";
    case 52: return "v2, bit fields (52 bytes)";
    case 56: return "v3, bit fields (56 bytes)";
    case 108: return "v4 (108 bytes)";
    case 124: return "v5 (124 bytes)";
    default: return "OS/2 2.x (" + std::to_string(hs) + " bytes)";
    }
}

std::string layout_name(const Header& h) {
    std::string s = std::to_string(h.bits_per_pixel) + "-bit";
    if (h.bits_per_pixel <= 8)
        s += " (" + std::to_string(h.colours_used ? h.colours_used : 1u << h.bits_per_pixel) + " colours)";
    if (h.compression == 1) s += ", RLE8";
    else if (h.compression == 2) s += ", RLE4";
    else if (h.compression == 3 || h.compression == 6) s += ", bit fields";
    return s;
}

bool looks_like(const uint8_t* d, std::size_t size) {
    if (size < 26 || d[0] != 'B' || d[1] != 'M') return false;
    uint32_t hs = u32(d + 14);
    return hs == 12 || (hs >= 16 && hs <= 124);
}

Image read(const uint8_t* d, std::size_t size) {
    if (size < 26) throw Error("too short for a bitmap header");
    if (d[0] != 'B' || d[1] != 'M') throw Error("not a bitmap (no BM)");
    Image img;
    Header& h = img.header;
    h.file_size = u32(d + 2);
    h.pixel_offset = u32(d + 10);
    h.header_size = u32(d + 14);
    const uint32_t hs = h.header_size;
    if (hs != 12 && (hs < 16 || hs > 124)) throw Error("unknown header size " + std::to_string(hs));
    if (14 + std::size_t(hs) > size) throw Error("the header runs past the end of the file");
    const uint8_t* b = d + 14;
    if (hs == 12) {
        h.width = int16_t(u16(b + 4));
        h.height = int16_t(u16(b + 6));
        h.planes = u16(b + 8);
        h.bits_per_pixel = u16(b + 10);
    } else {
        h.width = int32_t(u32(b + 4));
        h.height = int32_t(u32(b + 8));
        h.planes = u16(b + 12);
        h.bits_per_pixel = u16(b + 14);
        if (hs >= 20) h.compression = u32(b + 16);
        if (hs >= 24) h.image_size = u32(b + 20);
        if (hs >= 32) h.xppm = int32_t(u32(b + 24)), h.yppm = int32_t(u32(b + 28));
        if (hs >= 36) h.colours_used = u32(b + 32);
        if (hs >= 40) h.colours_important = u32(b + 36);
        // OS/2 2.x's compression 3 is Huffman 1D and 4 RLE24, not bit fields
        if (hs != 40 && hs != 52 && hs != 56 && hs != 108 && hs != 124 && h.compression >= 3)
            throw Error("unsupported OS/2 compression " + std::to_string(h.compression));
    }
    const int bpp = h.bits_per_pixel;
    if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
        throw Error("unsupported bit depth " + std::to_string(bpp));
    if (h.compression > 3 && h.compression != 6) throw Error("unsupported compression " + std::to_string(h.compression));
    if ((h.compression == 1 && bpp != 8) || (h.compression == 2 && bpp != 4) ||
        ((h.compression == 3 || h.compression == 6) && bpp != 16 && bpp != 32))
        throw Error("compression " + std::to_string(h.compression) + " with " + std::to_string(bpp) + " bits");
    if (h.width <= 0 || h.height == 0 || h.height == INT32_MIN) throw Error("empty image");
    const bool top_down = h.height < 0;
    img.width = h.width;
    img.height = top_down ? -h.height : h.height;
    if (std::size_t(img.width) * img.height > (1u << 26)) throw Error("implausibly large image");

    // Masks: in the v2+ header, or the three dwords after a 40-byte one.
    bool alpha_mask = false;
    if (h.compression == 3 || h.compression == 6) {
        const uint8_t* m = nullptr;
        if (hs >= 52) m = b + 40;
        else if (14 + 40 + 16 <= size) m = d + 14 + 40;
        if (!m) throw Error("the bit field masks run past the end of the file");
        h.masks[0] = u32(m), h.masks[1] = u32(m + 4), h.masks[2] = u32(m + 8);
        if (h.compression == 6 || hs >= 56) h.masks[3] = u32(m + 12);
    } else if (bpp == 16) {
        h.masks[0] = 0x7c00, h.masks[1] = 0x03e0, h.masks[2] = 0x001f;
    } else if (bpp == 32) {
        h.masks[0] = 0x00ff0000, h.masks[1] = 0x0000ff00, h.masks[2] = 0x000000ff;
        if (hs >= 56) h.masks[3] = u32(b + 52);   // v3+ headers carry an alpha mask even for BI_RGB
    }
    alpha_mask = h.masks[3] != 0;

    // The palette.
    uint8_t pal[256 * 4] = {};
    if (bpp <= 8) {
        int max = 1 << bpp;
        img.colours = h.colours_used && int(h.colours_used) < max ? int(h.colours_used) : max;
        int entry = hs == 12 ? 3 : 4;
        std::size_t at = 14 + hs + (h.compression == 3 && hs == 40 ? 12 : 0);
        for (int i = 0; i < img.colours && at + entry <= size; ++i, at += entry)
            pal[4 * i] = d[at + 2], pal[4 * i + 1] = d[at + 1], pal[4 * i + 2] = d[at];
    }

    // The pixels, as indices (<= 8 bits, decoded RLE) or raw rows.
    const std::size_t n = std::size_t(img.width) * img.height;
    img.rgba.assign(n * 4, 0);
    for (std::size_t i = 3; i < img.rgba.size(); i += 4) img.rgba[i] = 255;
    auto put = [&](int x, int row, const uint8_t* rgba) {   // row: as stored
        int y = top_down ? row : img.height - 1 - row;
        std::memcpy(&img.rgba[(std::size_t(y) * img.width + x) * 4], rgba, 4);
    };
    std::size_t p = h.pixel_offset;
    if (p > size) throw Error("the pixel data starts past the end of the file");

    if (h.compression == 1 || h.compression == 2) {
        bool four = h.compression == 2;
        int x = 0, row = 0;
        auto index = [&](int i) {
            uint8_t q[4] = {pal[4 * (i & 0xff)], pal[4 * (i & 0xff) + 1], pal[4 * (i & 0xff) + 2], 255};
            if (x < img.width && row < img.height) put(x, row, q);
            ++x;
        };
        bool done = false;
        while (!done && p + 1 < size && row < img.height) {
            int count = d[p], v = d[p + 1];
            p += 2;
            if (count) {
                for (int i = 0; i < count; ++i) index(four ? (i & 1 ? v & 15 : v >> 4) : v);
            } else if (v == 0) {
                x = 0, ++row;                           // end of line
            } else if (v == 1) {
                done = true;                            // end of bitmap
            } else if (v == 2) {
                if (p + 1 >= size) break;
                x += d[p], row += d[p + 1];             // delta
                p += 2;
            } else {
                int bytes = four ? (v + 1) / 2 : v;     // absolute run, padded to a word
                if (p + bytes > size) break;
                for (int i = 0; i < v; ++i) index(four ? (i & 1 ? d[p + i / 2] & 15 : d[p + i / 2] >> 4) : d[p + i]);
                p += (bytes + 1) & ~1;
            }
        }
        img.truncated = !done && row < img.height;
    } else {
        const std::size_t stride = ((std::size_t(img.width) * bpp + 31) / 32) * 4;
        for (int row = 0; row < img.height; ++row) {
            std::size_t at = p + stride * row;
            if (at + (std::size_t(img.width) * bpp + 7) / 8 > size) {
                img.truncated = true;
                break;
            }
            const uint8_t* s = d + at;
            for (int x = 0; x < img.width; ++x) {
                uint8_t q[4] = {0, 0, 0, 255};
                if (bpp <= 8) {
                    int per = 8 / bpp, shift = 8 - bpp * (x % per + 1);
                    int i = (s[x / per] >> shift) & ((1 << bpp) - 1);
                    std::memcpy(q, pal + 4 * i, 3);
                } else if (bpp == 24) {
                    q[0] = s[3 * x + 2], q[1] = s[3 * x + 1], q[2] = s[3 * x];
                } else {
                    uint32_t v = bpp == 16 ? u16(s + 2 * x) : u32(s + 4 * x);
                    q[0] = field(v, h.masks[0]), q[1] = field(v, h.masks[1]), q[2] = field(v, h.masks[2]);
                    if (alpha_mask) q[3] = field(v, h.masks[3]);
                    if (bpp == 32 && !alpha_mask && s[4 * x + 3]) img.unused_alpha = true;
                }
                put(x, row, q);
            }
        }
    }
    if (alpha_mask) {
        // An alpha mask over a channel that is all zero is no alpha at all
        // (common in v4/v5 bitmaps saved from opaque images).
        bool any = false;
        for (std::size_t i = 3; i < img.rgba.size() && !any; i += 4) any = img.rgba[i] != 0;
        if (!any)
            for (std::size_t i = 3; i < img.rgba.size(); i += 4) img.rgba[i] = 255;
        for (std::size_t i = 3; i < img.rgba.size() && !img.has_alpha; i += 4) img.has_alpha = img.rgba[i] != 255;
    }
    return img;
}

}  // namespace lithtech::bmp
