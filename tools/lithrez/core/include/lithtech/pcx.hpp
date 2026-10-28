// ZSoft PCX images: reading and decoding to RGBA. The rez archives of NOLF,
// NOLF2 and AvP2 carry them for interface art (NOLF2.REZ 251 of them,
// AVP2.REZ 376): 8-bit with a 256-colour palette at the end, 24-bit as
// three 8-bit planes, and the odd 16-colour EGA image (four 1-bit planes,
// the palette in the header). Every layout of the format is read:
//   bits/pixel x planes   pixels
//   1 x 1                 monochrome (header palette entries 0 and 1, or
//                         black and white)
//   1 x 2..4              EGA planar, 4..16 colours (the header palette)
//   2 x 1, 4 x 1          packed, 4 or 16 colours (the header palette)
//   8 x 1                 256 colours (the palette after the 0x0C marker at
//                         the end; without one, grey levels)
//   8 x 3, 8 x 4          RGB, RGBA (one plane per channel)
// Files of version 0 and 3 have no palette in the header: the EGA defaults
// stand in for it. The RLE stream is decoded as one run of bytes, so a run
// that crosses the end of a scanline or plane is read as written.

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace lithtech::pcx {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Header {
    uint8_t manufacturer = 0;      // 10
    uint8_t version = 0;           // 0, 2, 3, 4 or 5
    uint8_t encoding = 0;          // 1: RLE (0, uncompressed, is read too)
    uint8_t bits_per_pixel = 0;    // per plane
    uint16_t xmin = 0, ymin = 0, xmax = 0, ymax = 0;
    uint16_t hdpi = 0, vdpi = 0;
    uint8_t ega_palette[48] = {};
    uint8_t planes = 0;
    uint16_t bytes_per_line = 0;   // per plane
    uint16_t palette_info = 0;     // 1 colour, 2 greyscale
};

struct Image {
    Header header;
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;     // top row first
    // Colours: 0 for a true-colour image, else the palette's size.
    int colours = 0;
    // Where the palette came from: "VGA" (the 256 colours at the end),
    // "header", "EGA defaults", "greyscale" or "" (true colour).
    std::string palette;
    // The pixel data ran out before the image was complete (the rest is
    // black).
    bool truncated = false;
};

// Whether the bytes start like a PCX file (manufacturer 10, a known
// version, encoding 0 or 1, a sane layout).
bool looks_like(const uint8_t* data, std::size_t size);

// Throws Error if the file can't be read as a PCX image.
Image read(const uint8_t* data, std::size_t size);
inline Image read(const std::vector<uint8_t>& v) { return read(v.data(), v.size()); }

// "8-bit indexed", "24-bit RGB", "16-colour EGA (4 planes)", ...
std::string layout_name(const Header& h);

}  // namespace lithtech::pcx
