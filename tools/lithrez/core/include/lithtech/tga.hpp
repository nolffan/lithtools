// Truevision TGA images: reading and decoding to RGBA. NOLF.REZ carries one
// (TEX/FABRIC/FA04/FA0163.TGA), and the games' and tools' installs many
// loose ones: 24- and 32-bit, mostly bottom-up, one RLE. Every image type
// of the format is read:
//   1, 9    colour-mapped (raw, RLE): 8- or 16-bit indices into a map of
//           15-, 16-, 24- or 32-bit entries (the map's first index honoured)
//   2, 10   true colour (raw, RLE): 15, 16 (5-5-5, the top bit alpha when the
//           descriptor says so), 24 and 32 bits
//   3, 11   greyscale (raw, RLE): 8 bits, or 16 (grey and alpha)
// The origin bits (bottom-up or top-down, right-to-left) are applied, so
// the result is always top row first, left to right. Whether there is
// alpha: a TGA 2.0 file's extension area says (its attributes type: 0 or 1
// none, 2 or 3 alpha, 4 premultiplied alpha, read as stored); without one,
// the descriptor's alpha bits, and a 32-bit image's fourth byte even when
// they are zero (writers disagree on setting them). ImageMagick reads them
// the same way. alpha says which rule decided.

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace lithtech::tga {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Header {
    uint8_t id_length = 0;
    uint8_t colour_map_type = 0;     // 0 none, 1 present
    uint8_t image_type = 0;          // 1, 2, 3, 9, 10, 11
    uint16_t map_first = 0, map_length = 0;
    uint8_t map_entry_bits = 0;
    uint16_t x_origin = 0, y_origin = 0, width = 0, height = 0;
    uint8_t bits_per_pixel = 0;
    uint8_t descriptor = 0;          // bits 0-3 alpha bits, 4 right-to-left, 5 top-down
    int alpha_bits() const { return descriptor & 15; }
    bool top_down() const { return descriptor & 0x20; }
    bool right_to_left() const { return descriptor & 0x10; }
    bool rle() const { return image_type >= 9; }
};

struct Image {
    Header header;
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;       // top row first
    std::string id;                  // the image ID field, if any
    bool has_alpha = false;          // the alpha channel carries anything (not all 255)
    // "none", "8 alpha bits", "the fourth byte (no alpha bits declared)",
    // "none (TGA 2.0 attributes type 0)", "premultiplied (TGA 2.0)", ...
    std::string alpha;
    bool tga2 = false;               // a TGA 2.0 footer (TRUEVISION-XFILE)
    bool truncated = false;          // the pixel data ran out (the rest is black)
};

bool looks_like(const uint8_t* data, std::size_t size);

// Throws Error if the file can't be read as a TGA image.
Image read(const uint8_t* data, std::size_t size);
inline Image read(const std::vector<uint8_t>& v) { return read(v.data(), v.size()); }

// "true colour, 32-bit, RLE", "colour-mapped, 8-bit (256 x 24-bit)", ...
std::string layout_name(const Header& h);

}  // namespace lithtech::tga
