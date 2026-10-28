// Windows and OS/2 bitmaps (.bmp): reading and decoding to RGBA. None of
// the shipped rez archives carries one, but the games' and tools' installs
// have thousands (24-, 8-, 4- and 32-bit, a few top-down), and a rez built
// from a mod's folders may. Every layout of the format is read:
//   headers   OS/2 1.x (12 bytes), OS/2 2.x (16..64), Windows 3 (40),
//             v2/v3 bit fields (52, 56), v4 (108), v5 (124)
//   pixels    1, 4 and 8 bits through the palette (3-byte entries for OS/2
//             1.x, 4-byte otherwise; biClrUsed honoured), 16 bits (5-5-5,
//             or bit fields), 24 bits, 32 bits (bit fields, or B-G-R-x)
//   encoding  none, RLE8, RLE4 (with the delta and end-of-line codes), and
//             bit fields (BI_BITFIELDS, BI_ALPHABITFIELDS)
// Rows are bottom-up unless the height is negative. A 32-bit BI_RGB
// image's fourth byte is unused by the format, so it is read as opaque
// unless an alpha mask is given (v4/v5 headers, BI_ALPHABITFIELDS);
// unused_alpha says whether that byte held anything.

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace lithtech::bmp {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Header {
    uint32_t file_size = 0, pixel_offset = 0;
    uint32_t header_size = 0;        // 12, 16..64, 40, 52, 56, 108, 124
    int32_t width = 0, height = 0;   // height < 0: top-down
    uint16_t planes = 0, bits_per_pixel = 0;
    uint32_t compression = 0;        // 0 RGB, 1 RLE8, 2 RLE4, 3 BITFIELDS, 6 ALPHABITFIELDS
    uint32_t image_size = 0;
    int32_t xppm = 0, yppm = 0;      // pixels per metre
    uint32_t colours_used = 0, colours_important = 0;
    uint32_t masks[4] = {};          // R, G, B, A (bit fields, or the defaults)
};

struct Image {
    Header header;
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;       // top row first
    int colours = 0;                 // the palette's size (0: no palette)
    bool has_alpha = false;          // an alpha mask gave some pixel alpha below 255
    bool unused_alpha = false;       // a 32-bit BI_RGB image's fourth byte isn't all zero
    bool truncated = false;          // the pixel data ran out (the rest is black)
};

bool looks_like(const uint8_t* data, std::size_t size);

// Throws Error if the file can't be read as a bitmap.
Image read(const uint8_t* data, std::size_t size);
inline Image read(const std::vector<uint8_t>& v) { return read(v.data(), v.size()); }

// "24-bit", "8-bit (256 colours), RLE8", "32-bit, bit fields", ...
std::string layout_name(const Header& h);
// "Windows 3 (40 bytes)", "v5 (124 bytes)", "OS/2 1.x (12 bytes)", ...
std::string header_name(uint32_t header_size);

}  // namespace lithtech::bmp
