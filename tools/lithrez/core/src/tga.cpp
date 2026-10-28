#include <lithtech/tga.hpp>

#include <algorithm>
#include <cstring>

namespace lithtech::tga {

namespace {

uint16_t u16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }

Header parse_header(const uint8_t* d) {
    Header h;
    h.id_length = d[0];
    h.colour_map_type = d[1];
    h.image_type = d[2];
    h.map_first = u16(d + 3), h.map_length = u16(d + 5);
    h.map_entry_bits = d[7];
    h.x_origin = u16(d + 8), h.y_origin = u16(d + 10), h.width = u16(d + 12), h.height = u16(d + 14);
    h.bits_per_pixel = d[16];
    h.descriptor = d[17];
    return h;
}

const char* check(const Header& h) {
    int t = h.image_type & 7;
    if (h.image_type != 1 && h.image_type != 2 && h.image_type != 3 && h.image_type != 9 &&
        h.image_type != 10 && h.image_type != 11)
        return "unknown image type";
    if (h.colour_map_type > 1) return "unknown colour map type";
    if (t == 1) {
        if (h.colour_map_type != 1) return "a colour-mapped image without a colour map";
        if (h.bits_per_pixel != 8 && h.bits_per_pixel != 16) return "unsupported index size";
    } else if (t == 2) {
        if (h.bits_per_pixel != 15 && h.bits_per_pixel != 16 && h.bits_per_pixel != 24 && h.bits_per_pixel != 32)
            return "unsupported pixel size";
    } else if (h.bits_per_pixel != 8 && h.bits_per_pixel != 16) {
        return "unsupported greyscale pixel size";
    }
    if (h.colour_map_type == 1 && h.map_entry_bits != 15 && h.map_entry_bits != 16 && h.map_entry_bits != 24 &&
        h.map_entry_bits != 32)
        return "unsupported colour map entry size";
    if (h.width == 0 || h.height == 0) return "empty image";
    return nullptr;
}

// A 15/16-bit A1R5G5B5 value as RGBA.
void from16(uint16_t v, bool alpha, uint8_t* o) {
    int r = (v >> 10) & 31, g = (v >> 5) & 31, b = v & 31;
    o[0] = uint8_t(r << 3 | r >> 2), o[1] = uint8_t(g << 3 | g >> 2), o[2] = uint8_t(b << 3 | b >> 2);
    o[3] = alpha ? (v & 0x8000 ? 255 : 0) : 255;
}

}  // namespace

bool looks_like(const uint8_t* d, std::size_t size) {
    return size >= 18 && !check(parse_header(d));
}

std::string layout_name(const Header& h) {
    std::string s;
    int t = h.image_type & 7;
    if (t == 1)
        s = "colour-mapped, " + std::to_string(h.bits_per_pixel) + "-bit (" + std::to_string(h.map_length) + " x " +
            std::to_string(h.map_entry_bits) + "-bit)";
    else if (t == 2)
        s = "true colour, " + std::to_string(h.bits_per_pixel) + "-bit";
    else
        s = h.bits_per_pixel == 16 ? "greyscale with alpha, 16-bit" : "greyscale, 8-bit";
    if (h.rle()) s += ", RLE";
    return s;
}

Image read(const uint8_t* d, std::size_t size) {
    if (size < 18) throw Error("too short for a TGA header");
    Image img;
    Header& h = img.header;
    h = parse_header(d);
    if (const char* e = check(h)) throw Error(e);
    img.width = h.width, img.height = h.height;
    if (std::size_t(img.width) * img.height > (1u << 26)) throw Error("implausibly large image");
    std::size_t p = 18;
    if (p + h.id_length > size) throw Error("the image ID runs past the end of the file");
    img.id.assign(reinterpret_cast<const char*>(d + p), h.id_length);
    while (!img.id.empty() && img.id.back() == 0) img.id.pop_back();
    p += h.id_length;

    // The colour map, as RGBA.
    std::vector<uint8_t> map;
    if (h.colour_map_type == 1) {
        int eb = (h.map_entry_bits + 7) / 8;
        if (p + std::size_t(h.map_length) * eb > size) throw Error("the colour map runs past the end of the file");
        map.resize(std::size_t(h.map_length) * 4);
        for (int i = 0; i < h.map_length; ++i, p += eb) {
            uint8_t* o = &map[std::size_t(i) * 4];
            if (eb == 2) {
                from16(u16(d + p), h.map_entry_bits == 16 && h.alpha_bits(), o);
            } else {
                o[0] = d[p + 2], o[1] = d[p + 1], o[2] = d[p];
                o[3] = eb == 4 ? d[p + 3] : 255;
            }
        }
    }

    // The pixels, as stored (one element of bpp bits each), raw or RLE.
    const int bpp = (h.bits_per_pixel + 7) / 8;
    const std::size_t count = std::size_t(img.width) * img.height;
    std::vector<uint8_t> px(count * bpp);
    std::size_t o = 0;
    if (!h.rle()) {
        std::size_t k = std::min(px.size(), size > p ? size - p : 0);
        std::memcpy(px.data(), d + p, k);
        o = k;
    } else {
        while (o < px.size() && p < size) {
            uint8_t c = d[p++];
            int n = (c & 0x7f) + 1;
            if (c & 0x80) {
                if (p + bpp > size) break;
                for (int i = 0; i < n && o < px.size(); ++i, o += bpp) std::memcpy(&px[o], d + p, bpp);
                p += bpp;
            } else {
                std::size_t k = std::min({std::size_t(n) * bpp, px.size() - o, size - p});
                std::memcpy(&px[o], d + p, k);
                o += k, p += k;
                if (k < std::size_t(n) * bpp && o < px.size()) break;
            }
        }
    }
    img.truncated = o < px.size();

    const int t = h.image_type & 7;
    // Whether there is alpha (tga.hpp): the TGA 2.0 extension area, else the
    // descriptor's alpha bits, else a 32-bit image's fourth byte.
    bool keep_alpha = h.alpha_bits() > 0 || (t == 2 && h.bits_per_pixel == 32);
    img.alpha = h.alpha_bits() > 0 ? std::to_string(h.alpha_bits()) + " alpha bits"
                : keep_alpha       ? "the fourth byte (no alpha bits declared)"
                                   : "none";
    if (size >= 18 + 26 && !std::memcmp(d + size - 18, "TRUEVISION-XFILE.", 18)) {
        img.tga2 = true;
        uint32_t ext = uint32_t(d[size - 26] | d[size - 25] << 8 | d[size - 24] << 16 | uint32_t(d[size - 23]) << 24);
        if (ext >= 18 && ext + 495 <= size - 26 && u16(d + ext) >= 495) {
            int type = d[ext + 494];
            if (type == 0 || type == 1) {
                keep_alpha = false;
                img.alpha = "none (TGA 2.0 attributes type " + std::to_string(type) + ")";
            } else if (type == 2 || type == 3 || type == 4) {
                keep_alpha = true;
                img.alpha = type == 4 ? "premultiplied (TGA 2.0 attributes type 4)"
                                      : "alpha (TGA 2.0 attributes type " + std::to_string(type) + ")";
            }
        }
    }
    img.rgba.assign(count * 4, 0);
    for (int sy = 0; sy < img.height; ++sy) {
        int y = h.top_down() ? sy : img.height - 1 - sy;
        for (int sx = 0; sx < img.width; ++sx) {
            int x = h.right_to_left() ? img.width - 1 - sx : sx;
            const uint8_t* s = &px[(std::size_t(sy) * img.width + sx) * bpp];
            uint8_t* q = &img.rgba[(std::size_t(y) * img.width + x) * 4];
            if (std::size_t(s - px.data()) + bpp > o) {   // past the end of a truncated file: black
                q[3] = 255;
                continue;
            }
            if (t == 1) {
                int index = (bpp == 2 ? u16(s) : s[0]) - h.map_first;
                if (index >= 0 && index < h.map_length) std::memcpy(q, &map[std::size_t(index) * 4], 4);
                else q[3] = 255;
            } else if (t == 2) {
                if (bpp == 2) {
                    from16(u16(s), h.bits_per_pixel == 16 && keep_alpha && h.alpha_bits(), q);
                } else {
                    q[0] = s[2], q[1] = s[1], q[2] = s[0];
                    q[3] = bpp == 4 && keep_alpha ? s[3] : 255;
                }
            } else {
                q[0] = q[1] = q[2] = s[0];
                q[3] = bpp == 2 ? s[1] : 255;
            }
        }
    }
    for (std::size_t i = 3; i < img.rgba.size(); i += 4)
        if (img.rgba[i] != 255) {
            img.has_alpha = true;
            break;
        }
    return img;
}

}  // namespace lithtech::tga
