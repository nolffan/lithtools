// lithtech::tga and lithtech::bmp on images made here, one per layout,
// each decoded and checked pixel by pixel.
//   TGA: true colour 24/32/16-bit (raw and RLE), colour-mapped (with a
//        first index), greyscale and grey+alpha, all four origins,
//        truncated, refused.
//   BMP: 24-bit bottom-up and top-down, 32-bit BI_RGB (fourth byte unused)
//        and with an alpha mask, 16-bit 5-5-5 and 5-6-5 bit fields, 8/4/1-bit
//        palettes (biClrUsed), RLE8 and RLE4 (delta, absolute runs), OS/2
//        1.x, truncated, refused.
//   image_test
#include <lithtech/bmp.hpp>
#include <lithtech/tga.hpp>

#include <cstdio>
#include <cstring>
#include <initializer_list>
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

using Bytes = std::vector<uint8_t>;
static void put16(Bytes& b, int v) { b.push_back(uint8_t(v)), b.push_back(uint8_t(v >> 8)); }
static void put32(Bytes& b, uint32_t v) { put16(b, int(v & 0xffff)), put16(b, int(v >> 16)); }
static void add(Bytes& b, std::initializer_list<int> v) { for (int x : v) b.push_back(uint8_t(x)); }

template <class Img>
static bool px(const Img& img, int x, int y, int r, int g, int b, int a = 255) {
    const uint8_t* p = img.rgba.data() + (std::size_t(y) * img.width + x) * 4;
    return p[0] == r && p[1] == g && p[2] == b && p[3] == a;
}

// ---- TGA ------------------------------------------------------------------

static Bytes make_tga(int type, int bpp, int w, int h, int desc, const Bytes& pixels, int cmtype = 0, int first = 0,
                 int maplen = 0, int entry = 0, const Bytes& map = {}, const char* id = "") {
    Bytes b;
    add(b, {int(std::strlen(id)), cmtype, type});
    put16(b, first), put16(b, maplen), b.push_back(uint8_t(entry));
    put16(b, 0), put16(b, 0), put16(b, w), put16(b, h);
    add(b, {bpp, desc});
    b.insert(b.end(), id, id + std::strlen(id));
    b.insert(b.end(), map.begin(), map.end());
    b.insert(b.end(), pixels.begin(), pixels.end());
    return b;
}

static void test_tga() {
    // 24-bit, bottom-up (the default): the first row stored is the bottom one.
    {
        Bytes p = {1, 2, 3, 4, 5, 6,   10, 20, 30, 40, 50, 60};   // BGR BGR (bottom), BGR BGR (top)
        auto img = tga::read(make_tga(2, 24, 2, 2, 0, p, 0, 0, 0, 0, {}, "hello"));
        CHECK(img.width == 2 && img.height == 2 && img.id == "hello" && !img.has_alpha);
        CHECK(px(img, 0, 1, 3, 2, 1) && px(img, 1, 1, 6, 5, 4) && px(img, 0, 0, 30, 20, 10));
        CHECK(tga::layout_name(img.header) == "true colour, 24-bit");
    }
    // 32-bit, top-down, 8 alpha bits.
    {
        auto img = tga::read(make_tga(2, 32, 2, 1, 0x28, {1, 2, 3, 128, 4, 5, 6, 255}));
        CHECK(img.has_alpha && px(img, 0, 0, 3, 2, 1, 128) && px(img, 1, 0, 6, 5, 4));
    }
    // 32-bit declaring no alpha bits: the fourth byte is still alpha...
    {
        auto img = tga::read(make_tga(2, 32, 1, 1, 0x20, {1, 2, 3, 7}));
        CHECK(px(img, 0, 0, 3, 2, 1, 7) && img.alpha == "the fourth byte (no alpha bits declared)");
        // ...unless a TGA 2.0 extension area says there is none
        Bytes f = make_tga(2, 32, 1, 1, 0x28, {1, 2, 3, 7});
        uint32_t ext = uint32_t(f.size());
        Bytes area(495, 0);
        area[0] = 0xef, area[1] = 1;     // its size, 495
        area[494] = 0;                   // attributes type 0: no alpha
        f.insert(f.end(), area.begin(), area.end());
        put32(f, ext), put32(f, 0);
        const char sig[] = "TRUEVISION-XFILE.";
        f.insert(f.end(), sig, sig + 18);
        auto v2 = tga::read(f);
        CHECK(v2.tga2 && px(v2, 0, 0, 3, 2, 1) && !v2.has_alpha && v2.alpha == "none (TGA 2.0 attributes type 0)");
    }
    // Right-to-left, top-down.
    {
        auto img = tga::read(make_tga(2, 24, 2, 1, 0x30, {1, 1, 1, 2, 2, 2}));
        CHECK(px(img, 0, 0, 2, 2, 2) && px(img, 1, 0, 1, 1, 1));
    }
    // 16-bit A1R5G5B5 with one alpha bit.
    {
        Bytes p;
        put16(p, 0x8000 | 31 << 10), put16(p, 31);   // opaque red, transparent blue
        auto img = tga::read(make_tga(2, 16, 2, 1, 0x21, p));
        CHECK(px(img, 0, 0, 255, 0, 0) && px(img, 1, 0, 0, 0, 255, 0));
    }
    // RLE true colour: a run of 3 and a raw packet of 1, across rows.
    {
        Bytes p = {0x82, 9, 8, 7, 0x00, 1, 2, 3};
        auto img = tga::read(make_tga(10, 24, 2, 2, 0x20, p));
        CHECK(img.header.rle() && !img.truncated);
        CHECK(px(img, 0, 0, 7, 8, 9) && px(img, 1, 0, 7, 8, 9) && px(img, 0, 1, 7, 8, 9) && px(img, 1, 1, 3, 2, 1));
        CHECK(tga::layout_name(img.header) == "true colour, 24-bit, RLE");
    }
    // Colour-mapped: 8-bit indices into a 24-bit map starting at index 10.
    {
        Bytes map = {0, 0, 255, 0, 255, 0};   // 10: red, 11: green (BGR)
        auto img = tga::read(make_tga(1, 8, 2, 1, 0x20, {11, 10}, 1, 10, 2, 24, map));
        CHECK(px(img, 0, 0, 0, 255, 0) && px(img, 1, 0, 255, 0, 0));
        // RLE colour-mapped
        auto rle = tga::read(make_tga(9, 8, 3, 1, 0x20, {0x82, 10}, 1, 10, 2, 24, map));
        CHECK(px(rle, 2, 0, 255, 0, 0));
    }
    // Greyscale, and grey with alpha.
    {
        auto g = tga::read(make_tga(3, 8, 1, 1, 0, {77}));
        CHECK(px(g, 0, 0, 77, 77, 77));
        auto ga = tga::read(make_tga(11, 16, 2, 1, 0x28, {0x81, 50, 60}));
        CHECK(px(ga, 0, 0, 50, 50, 50, 60) && px(ga, 1, 0, 50, 50, 50, 60) && ga.has_alpha);
    }
    // Truncated: the rest is black.
    {
        auto img = tga::read(make_tga(2, 24, 2, 1, 0x20, {1, 2, 3, 4}));
        CHECK(img.truncated && px(img, 0, 0, 3, 2, 1) && px(img, 1, 0, 0, 0, 0));
    }
    // Refused.
    {
        auto refused = [](const Bytes& b) {
            try { tga::read(b); return false; } catch (const tga::Error&) { return true; }
        };
        CHECK(refused(Bytes(10)));
        CHECK(refused(make_tga(4, 24, 1, 1, 0, {1, 2, 3})));                  // no type 4
        CHECK(refused(make_tga(1, 8, 1, 1, 0, {0})));                         // mapped, no map
        CHECK(refused(make_tga(2, 12, 1, 1, 0, {0, 0})));                     // 12-bit
        CHECK(refused(make_tga(2, 24, 0, 1, 0, {})));                         // empty
        CHECK(tga::looks_like(make_tga(2, 24, 1, 1, 0, {1, 2, 3}).data(), 21));
    }
}

// ---- BMP ------------------------------------------------------------------

// A bitmap: a 40-byte (or hs-byte) header, then extra (masks, palette), then
// the pixel rows as given.
static Bytes make_bmp(int w, int h, int bpp, uint32_t comp, const Bytes& extra, const Bytes& pixels, uint32_t used = 0,
                 int hs = 40) {
    Bytes b = {'B', 'M'};
    uint32_t off = 14 + uint32_t(hs) + uint32_t(extra.size());
    put32(b, off + uint32_t(pixels.size())), put32(b, 0), put32(b, off);
    put32(b, uint32_t(hs));
    if (hs == 12) {
        put16(b, w), put16(b, h), put16(b, 1), put16(b, bpp);
    } else {
        put32(b, uint32_t(w)), put32(b, uint32_t(h)), put16(b, 1), put16(b, bpp), put32(b, comp);
        put32(b, uint32_t(pixels.size())), put32(b, 2835), put32(b, 2835), put32(b, used), put32(b, 0);
        while (b.size() < 14 + std::size_t(hs)) b.push_back(0);
    }
    b.insert(b.end(), extra.begin(), extra.end());
    b.insert(b.end(), pixels.begin(), pixels.end());
    return b;
}

static void test_bmp() {
    // 24-bit, bottom-up, rows padded to 4 bytes.
    {
        Bytes p = {1, 2, 3, 0,   4, 5, 6, 0};   // 1x2: bottom row BGR + pad, top row
        auto img = bmp::read(make_bmp(1, 2, 24, 0, {}, p));
        CHECK(px(img, 0, 1, 3, 2, 1) && px(img, 0, 0, 6, 5, 4) && !img.truncated);
        CHECK(bmp::layout_name(img.header) == "24-bit" && bmp::header_name(40) == "Windows 3 (40 bytes)");
        // top-down
        auto td = bmp::read(make_bmp(1, -2, 24, 0, {}, p));
        CHECK(td.height == 2 && px(td, 0, 0, 3, 2, 1) && px(td, 0, 1, 6, 5, 4));
    }
    // 32-bit BI_RGB: the fourth byte is unused, read as opaque.
    {
        auto img = bmp::read(make_bmp(1, 1, 32, 0, {}, {1, 2, 3, 9}));
        CHECK(px(img, 0, 0, 3, 2, 1) && img.unused_alpha && !img.has_alpha);
    }
    // 32-bit with an alpha mask (BI_ALPHABITFIELDS).
    {
        Bytes m;
        put32(m, 0x00ff0000), put32(m, 0x0000ff00), put32(m, 0x000000ff), put32(m, 0xff000000);
        auto img = bmp::read(make_bmp(2, 1, 32, 6, m, {1, 2, 3, 64, 4, 5, 6, 255}));
        CHECK(px(img, 0, 0, 3, 2, 1, 64) && px(img, 1, 0, 6, 5, 4) && img.has_alpha);
        // an alpha mask over all-zero alpha is opaque
        auto zero = bmp::read(make_bmp(1, 1, 32, 6, m, {1, 2, 3, 0}));
        CHECK(px(zero, 0, 0, 3, 2, 1) && !zero.has_alpha);
    }
    // 16-bit: 5-5-5 by default, 5-6-5 by bit fields.
    {
        Bytes p;
        put16(p, 31 << 10), put16(p, 0);   // red, then the row's padding
        CHECK(px(bmp::read(make_bmp(1, 1, 16, 0, {}, p)), 0, 0, 255, 0, 0));
        Bytes m;
        put32(m, 0xf800), put32(m, 0x07e0), put32(m, 0x001f);
        Bytes q;
        put16(q, 0x07e0), put16(q, 0);     // green
        auto img = bmp::read(make_bmp(1, 1, 16, 3, m, q));
        CHECK(px(img, 0, 0, 0, 255, 0) && bmp::layout_name(img.header) == "16-bit, bit fields");
        Bytes r;
        put16(r, 16 << 11), put16(r, 0);   // red at 16/31: 10000 -> 10000100
        CHECK(px(bmp::read(make_bmp(1, 1, 16, 3, m, r)), 0, 0, 0x84, 0, 0));
    }
    // 8-bit with 2 colours used.
    {
        Bytes pal = {0, 0, 255, 0,   255, 0, 0, 0};   // 0 red, 1 blue (BGRx)
        auto img = bmp::read(make_bmp(3, 1, 8, 0, pal, {1, 0, 1, 0}, 2));
        CHECK(img.colours == 2 && px(img, 0, 0, 0, 0, 255) && px(img, 1, 0, 255, 0, 0));
        CHECK(bmp::layout_name(img.header) == "8-bit (2 colours)");
    }
    // 4-bit and 1-bit.
    {
        Bytes pal(16 * 4, 0);
        pal[4 * 5 + 2] = 50, pal[4 * 12 + 1] = 120;
        auto four = bmp::read(make_bmp(2, 1, 4, 0, pal, {0x5c, 0, 0, 0}));
        CHECK(px(four, 0, 0, 50, 0, 0) && px(four, 1, 0, 0, 120, 0));
        Bytes mono = {0, 0, 0, 0, 255, 255, 255, 0};
        auto one = bmp::read(make_bmp(9, 1, 1, 0, mono, {0x80, 0x80, 0, 0}));
        CHECK(px(one, 0, 0, 255, 255, 255) && px(one, 1, 0, 0, 0, 0) && px(one, 8, 0, 255, 255, 255));
    }
    // RLE8: an encoded run, an absolute run (odd: padded), end of line, a
    // delta, end of bitmap.
    {
        Bytes pal(256 * 4, 0);
        for (int i = 0; i < 256; ++i) pal[4 * i + 2] = uint8_t(i);   // index i: red i
        Bytes p = {2, 7,   0, 3, 1, 2, 3, 0,   0, 0,   0, 2, 1, 0,   1, 9,   0, 1};
        auto img = bmp::read(make_bmp(5, 2, 8, 1, pal, p));
        // bottom row (stored first): 7 7 1 2 3; top row: delta (+1, 0) then 9
        CHECK(px(img, 0, 1, 7, 0, 0) && px(img, 1, 1, 7, 0, 0) && px(img, 4, 1, 3, 0, 0));
        CHECK(px(img, 0, 0, 0, 0, 0) && px(img, 1, 0, 9, 0, 0) && !img.truncated);
        CHECK(bmp::layout_name(img.header) == "8-bit (256 colours), RLE8");
    }
    // RLE4: an encoded run alternates the two nibbles; an absolute run.
    {
        Bytes pal(16 * 4, 0);
        for (int i = 0; i < 16; ++i) pal[4 * i + 1] = uint8_t(i * 16);
        Bytes p = {3, 0x12,   0, 3, 0x45, 0x60,   0, 1};
        auto img = bmp::read(make_bmp(6, 1, 4, 2, pal, p));
        CHECK(px(img, 0, 0, 0, 16, 0) && px(img, 1, 0, 0, 32, 0) && px(img, 2, 0, 0, 16, 0));
        CHECK(px(img, 3, 0, 0, 64, 0) && px(img, 4, 0, 0, 80, 0) && px(img, 5, 0, 0, 96, 0));
    }
    // OS/2 1.x: a 12-byte header and 3-byte palette entries.
    {
        Bytes pal = {0, 0, 0,   0, 200, 0};
        auto img = bmp::read(make_bmp(2, 1, 1, 0, pal, {0x40, 0, 0, 0}, 0, 12));
        CHECK(px(img, 0, 0, 0, 0, 0) && px(img, 1, 0, 0, 200, 0));
        CHECK(bmp::header_name(img.header.header_size) == "OS/2 1.x (12 bytes)");
    }
    // A v5 header (124 bytes).
    {
        auto img = bmp::read(make_bmp(1, 1, 24, 0, {}, {1, 2, 3, 0}, 0, 124));
        CHECK(px(img, 0, 0, 3, 2, 1) && bmp::header_name(124) == "v5 (124 bytes)");
    }
    // Truncated.
    {
        auto img = bmp::read(make_bmp(1, 2, 24, 0, {}, {1, 2, 3, 0}));
        CHECK(img.truncated && px(img, 0, 1, 3, 2, 1) && px(img, 0, 0, 0, 0, 0));
    }
    // Refused.
    {
        auto refused = [](const Bytes& b) {
            try { bmp::read(b); return false; } catch (const bmp::Error&) { return true; }
        };
        CHECK(refused(Bytes(20)));
        Bytes f = make_bmp(1, 1, 24, 0, {}, {1, 2, 3, 0});
        f[0] = 'X';
        CHECK(refused(f) && !bmp::looks_like(f.data(), f.size()));
        CHECK(refused(make_bmp(1, 1, 12, 0, {}, {0, 0, 0, 0})));            // 12-bit
        CHECK(refused(make_bmp(1, 1, 24, 1, {}, {0, 0, 0, 0})));            // RLE8 on 24 bits
        CHECK(refused(make_bmp(0, 1, 24, 0, {}, {})));                      // empty
    }
}

int main() {
    test_tga();
    test_bmp();
    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
