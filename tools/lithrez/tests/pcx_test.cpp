// lithtech::pcx on images made here, one per layout, each decoded and
// checked pixel by pixel: 8-bit with a VGA palette (and an RLE run that
// crosses scanlines), 8-bit without one (grey levels), 24-bit and 32-bit
// planar, 16-colour EGA planes, monochrome, 2- and 4-bit packed, an
// uncompressed file, a truncated one, and files that must be refused.
//   pcx_test
#include <lithtech/pcx.hpp>

#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

using namespace lithtech;

static int failures = 0;
#define CHECK(c)                                                             \
    do {                                                                     \
        if (!(c)) {                                                          \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);         \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

struct Px { uint8_t r, g, b, a; };

// A PCX file: the header, the scanlines (each `planes` planes of `bpl`
// bytes, already packed) RLE-encoded as one stream unless raw, and an
// optional VGA palette.
static std::vector<uint8_t> make(int version, int bpp, int planes, int w, int h, int bpl,
                                 const std::vector<uint8_t>& lines, const uint8_t* ega = nullptr,
                                 const uint8_t* vga = nullptr, bool raw = false) {
    std::vector<uint8_t> f(128, 0);
    f[0] = 10, f[1] = uint8_t(version), f[2] = raw ? 0 : 1, f[3] = uint8_t(bpp);
    auto put16 = [&](int o, int v) { f[o] = uint8_t(v), f[o + 1] = uint8_t(v >> 8); };
    put16(4, 0), put16(6, 0), put16(8, w - 1), put16(10, h - 1), put16(12, 72), put16(14, 72);
    if (ega) std::memcpy(f.data() + 16, ega, 48);
    f[65] = uint8_t(planes);
    put16(66, bpl);
    put16(68, 1);
    if (raw) {
        f.insert(f.end(), lines.begin(), lines.end());
    } else {
        for (std::size_t i = 0; i < lines.size();) {
            std::size_t n = 1;
            while (i + n < lines.size() && lines[i + n] == lines[i] && n < 63) ++n;
            if (n > 1 || (lines[i] & 0xc0) == 0xc0) {
                f.push_back(uint8_t(0xc0 | n));
                f.push_back(lines[i]);
            } else {
                f.push_back(lines[i]);
            }
            i += n;
        }
    }
    if (vga) {
        f.push_back(12);
        f.insert(f.end(), vga, vga + 768);
    }
    return f;
}

static Px at(const pcx::Image& img, int x, int y) {
    const uint8_t* p = img.rgba.data() + (std::size_t(y) * img.width + x) * 4;
    return {p[0], p[1], p[2], p[3]};
}
static bool is(Px p, int r, int g, int b, int a = 255) { return p.r == r && p.g == g && p.b == b && p.a == a; }

int main() {
    uint8_t vga[768];
    for (int i = 0; i < 256; ++i) vga[3 * i] = uint8_t(i), vga[3 * i + 1] = uint8_t(255 - i), vga[3 * i + 2] = uint8_t(i ^ 0x55);

    // 8-bit, VGA palette; 3x2 with bpl 4 (padded). The run of 5 at the end
    // of row 0 crosses into row 1 (as some writers do).
    {
        std::vector<uint8_t> lines = {1, 2, 200, 9, 9, 9, 9, 9};   // row 0: 1 2 200 [pad 9]; row 1: 9 9 9 [pad 9]
        auto f = make(5, 8, 1, 3, 2, 4, lines, nullptr, vga);
        auto img = pcx::read(f);
        CHECK(img.width == 3 && img.height == 2 && img.colours == 256 && img.palette == "VGA" && !img.truncated);
        CHECK(is(at(img, 0, 0), 1, 254, 1 ^ 0x55));
        CHECK(is(at(img, 2, 0), 200, 55, 200 ^ 0x55));
        CHECK(is(at(img, 2, 1), 9, 246, 9 ^ 0x55));
        CHECK(pcx::layout_name(img.header) == "8-bit indexed");
    }
    // 8-bit without a palette: grey levels.
    {
        auto img = pcx::read(make(5, 8, 1, 2, 1, 2, {0, 77}));
        CHECK(img.palette == "greyscale" && is(at(img, 1, 0), 77, 77, 77));
    }
    // 24-bit: three planes per line.
    {
        std::vector<uint8_t> lines = {10, 11, 20, 21, 30, 31,     // row 0: R R, G G, B B
                                      40, 41, 50, 51, 60, 61};
        auto img = pcx::read(make(5, 8, 3, 2, 2, 2, lines));
        CHECK(img.colours == 0 && is(at(img, 0, 0), 10, 20, 30) && is(at(img, 1, 1), 41, 51, 61));
        CHECK(pcx::layout_name(img.header) == "24-bit RGB");
    }
    // 32-bit: a fourth plane is alpha.
    {
        auto img = pcx::read(make(5, 8, 4, 1, 1, 2, {1, 0, 2, 0, 3, 0, 128, 0}));
        CHECK(is(at(img, 0, 0), 1, 2, 3, 128));
    }
    // EGA, 4 planes of 1 bit: pixel x's index is bit (7 - x) of each plane.
    {
        uint8_t ega[48];
        for (int i = 0; i < 16; ++i) ega[3 * i] = uint8_t(i * 16), ega[3 * i + 1] = uint8_t(i), ega[3 * i + 2] = 7;
        // 8 pixels with indices 0..7: plane k has bit k of each
        std::vector<uint8_t> lines(4 * 2, 0);   // bpl 2
        for (int x = 0; x < 8; ++x)
            for (int k = 0; k < 4; ++k)
                if ((x >> k) & 1) lines[k * 2] |= uint8_t(0x80 >> x);
        auto img = pcx::read(make(5, 1, 4, 8, 1, 2, lines, ega));
        CHECK(img.colours == 16 && img.palette == "header");
        bool all = true;
        for (int x = 0; x < 8; ++x) all &= is(at(img, x, 0), x * 16, x, 7);
        CHECK(all);
        CHECK(pcx::layout_name(img.header) == "16-colour EGA (4 planes)");
        // version 3: no palette in the header, the EGA defaults
        auto def = pcx::read(make(3, 1, 4, 8, 1, 2, lines, ega));
        CHECK(def.palette == "EGA defaults" && is(at(def, 1, 0), 0, 0, 0xaa) && is(at(def, 7, 0), 0xaa, 0xaa, 0xaa));
    }
    // Monochrome, no colours in the header: 0 black, 1 white.
    {
        auto img = pcx::read(make(5, 1, 1, 10, 1, 2, {0xa0, 0x40}));   // 1010 0000  0100 0000
        CHECK(img.colours == 2 && is(at(img, 0, 0), 255, 255, 255) && is(at(img, 1, 0), 0, 0, 0) &&
              is(at(img, 2, 0), 255, 255, 255) && is(at(img, 9, 0), 255, 255, 255) && is(at(img, 8, 0), 0, 0, 0));
    }
    // Packed: 2 bits (4 colours) and 4 bits (16), header palette.
    {
        uint8_t ega[48] = {};
        for (int i = 0; i < 16; ++i) ega[3 * i] = uint8_t(100 + i);
        auto two = pcx::read(make(5, 2, 1, 4, 1, 2, {0x1b, 0}, ega));   // 00 01 10 11
        CHECK(two.colours == 4 && is(at(two, 0, 0), 100, 0, 0) && is(at(two, 3, 0), 103, 0, 0));
        auto four = pcx::read(make(5, 4, 1, 3, 1, 2, {0x5c, 0xf0}, ega));   // 5 12 15
        CHECK(four.colours == 16 && is(at(four, 0, 0), 105, 0, 0) && is(at(four, 1, 0), 112, 0, 0) &&
              is(at(four, 2, 0), 115, 0, 0));
    }
    // Uncompressed (encoding 0).
    {
        auto img = pcx::read(make(5, 8, 1, 2, 1, 2, {0xc5, 0xc6}, nullptr, vga, true));
        CHECK(is(at(img, 0, 0), 0xc5, 255 - 0xc5, 0xc5 ^ 0x55) && is(at(img, 1, 0), 0xc6, 255 - 0xc6, 0xc6 ^ 0x55));
    }
    // Truncated: the rest is black, and it says so.
    {
        // rows: R 1 2, G 3 4, B 5 6 / R 7 8, G 9 10, B 11 12; the last byte lost
        auto f = make(5, 8, 3, 2, 2, 2, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
        f.resize(f.size() - 1);
        auto img = pcx::read(f);
        CHECK(img.truncated && is(at(img, 0, 0), 1, 3, 5) && is(at(img, 1, 1), 8, 10, 0));
    }
    // Refused.
    {
        auto refused = [](const std::vector<uint8_t>& f) {
            try {
                pcx::read(f);
                return false;
            } catch (const pcx::Error&) {
                return true;
            }
        };
        CHECK(refused(std::vector<uint8_t>(64, 10)));                         // too short
        auto f = make(5, 8, 1, 2, 1, 2, {1, 2});
        f[0] = 9;
        CHECK(refused(f) && !pcx::looks_like(f.data(), f.size()));            // not ZSoft's
        f = make(5, 8, 2, 2, 1, 2, {1, 2, 3, 4});
        CHECK(refused(f));                                                    // 8 bits x 2 planes
        f = make(5, 8, 1, 9, 1, 2, {1, 2});
        CHECK(refused(f));                                                    // wider than its lines
        f = make(5, 8, 3, 2, 2, 2, std::vector<uint8_t>(12));
        CHECK(pcx::looks_like(f.data(), f.size()));
    }

    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
