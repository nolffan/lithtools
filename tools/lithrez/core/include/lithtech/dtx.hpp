// LithTech .dtx textures: reading and decoding to RGBA. Version -5 is every
// shipped file from NOLF to Tron 2.0; versions -2 (LithTech 1.0, Blood 2)
// and -3 (LithTech 1.5, KISS Psycho Circus) are 8-bit palettized with
// optional 4-bit alpha masks, read only. See plugins/blender/docs/DTX.md for
// the formats and the evidence.

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace lithtech::dtx {

enum Bpp : uint8_t {
    BPP_8P = 0, BPP_8 = 1, BPP_16 = 2, BPP_32 = 3,
    BPP_S3TC_DXT1 = 4, BPP_S3TC_DXT3 = 5, BPP_S3TC_DXT5 = 6, BPP_32P = 7,
};

enum Flags : uint32_t {
    DTX_FULLBRITE = 1u << 0, DTX_PREFER16BIT = 1u << 1, DTX_SECTIONSFIXED = 1u << 3,
    DTX_NOSYSCACHE = 1u << 6, DTX_PREFER4444 = 1u << 7, DTX_PREFER5551 = 1u << 8,
    DTX_32BITSYSCOPY = 1u << 9, DTX_CUBEMAP = 1u << 10, DTX_BUMPMAP = 1u << 11,
    DTX_LUMBUMPMAP = 1u << 12,
};

// Versions -2 and -3 share bits 0 (FULLBRITE) and 3 (SECTIONSFIXED) with
// -5; bit 1 is the alpha masks, and bit 5 keeps the renderer off 8-bit
// paletted surfaces (it converts to 16-bit instead).
enum OldFlags : uint32_t {
    DTX_OLD_ALPHAMASKS = 1u << 1,
    DTX_OLD_NOPALETTED = 1u << 5,
};

// In -2/-3 textures the renderer gives palette entries 246-255 the alpha
// bits of every 16-bit format it builds: they are always opaque, even where
// an alpha mask says otherwise.
constexpr int kOldOpaqueFirst = 246;

// How BPP_32P palette alpha is read. The files are the same on both
// platforms; what differs is how each engine draws them:
//  Stored: as stored, as the PC engine draws them (CC_32PtoBF copies the
//          palette's alpha).
//  Ps2:    as the PS2 draws them. DEdit's "Convert to 32P", which made the
//          PS2's textures, stores alpha halved (LTTexture's
//          Convert32BitTo8BitPalletized: a/2, so opaque is 0x7f) because
//          the PS2's GS takes 0x80 as opaque. Alpha is read back as it was
//          authored: 0x7f and up is opaque (255), below that a * 255 / 127
//          rounded; a palette whose alpha is all zero (made from a texture
//          with no alpha) is opaque.
// See plugins/blender/docs/DTX.md, "PS2".
enum class PaletteAlpha { Stored, Ps2 };

// The PS2 reading of a 256-entry palette's alpha (every 4th byte), in place.
void ps2_palette_alpha(uint8_t* palette);

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Section {
    std::string type, name;
    std::vector<uint8_t> data;
};

struct Image {
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;   // top row first
};

struct Texture {
    uint32_t res_type = 0;
    int32_t version = -5;
    int width = 0, height = 0;
    int mip_count = 0, section_count = 0;
    uint32_t iflags = 0, user_flags = 0;
    uint8_t extra[12] = {};
    std::string command;
    std::vector<std::vector<uint8_t>> mips;
    std::vector<Section> sections;
    bool truncated = false;

    // Versions -2/-3 only. extra holds header bytes 24-35 (the 36-byte -2
    // header has no command string; the -3 one has -5's 128 bytes, but no
    // shipped file holds text there, so command stays empty unless it is
    // printable).
    uint32_t palette_id[2] = {};               // the engine's palette cache key
    std::vector<uint8_t> palette;              // 256 x {unused, R, G, B}
    std::vector<std::vector<uint8_t>> masks;   // per level, DTX_OLD_ALPHAMASKS: rows of w/2 bytes, low nibble first

    bool is_old() const { return version == -2 || version == -3; }
    bool has_alpha_masks() const { return is_old() && (iflags & DTX_OLD_ALPHAMASKS); }
    // -3: the detail texture's scale (a plain float, not -5's value - 1) and
    // angle, header bytes 28 and 32.
    float old_detail_scale() const;
    int old_detail_angle() const { return uint16_t(extra[8] | extra[9] << 8); }

    Bpp bpp() const { return is_old() ? BPP_8P : Bpp(extra[2] ? extra[2] : uint8_t(BPP_32)); }
    int texture_group() const { return extra[0]; }
    int mipmaps_to_use() const { return extra[1]; }
    int non_s3tc_mip_offset() const { return extra[3]; }
    int ui_mip_offset() const { return extra[4]; }
    int priority() const { return extra[5]; }
    float detail_scale() const;
    int detail_angle() const;
    bool is_cubemap() const { return !is_old() && (iflags & DTX_CUBEMAP); }
    const Section* find_section(const std::string& type) const;

    // One level (and cube face 0-5) as RGBA.
    Image decode(int mip = 0, int face = 0, PaletteAlpha alpha = PaletteAlpha::Stored) const;
    // BPP_32P with no palette alpha above 0x80: made by DEdit's "Convert to
    // 32P" for the PS2 (every PS2 32P texture, and the nine 32P files that
    // went out with the PC games too).
    bool ps2_style_palette() const;
};

const char* bpp_name(Bpp b);
std::size_t image_size(Bpp bpp, int width, int height);

// Bytes of a -2/-3 level's alpha mask (dtx_Alloc: n/2, but 1 for a 1x1 level).
std::size_t old_mask_size(int width, int height);

// Versions -5, -3 and -2.
// lenient: keep a file that ends inside its pixels (two Tron prototypes).
Texture read(const std::vector<uint8_t>& data, bool lenient = true);
Texture read_file(const std::string& path, bool lenient = true);

// RGBA from one level's stored pixels.
Image decode_pixels(Bpp bpp, int width, int height, const uint8_t* data, std::size_t size,
                    const uint8_t* palette_bgra = nullptr, uint32_t iflags = 0,
                    PaletteAlpha alpha = PaletteAlpha::Stored);

// RGBA from one -2/-3 level: palette entries {unused, R, G, B}; with a mask,
// alpha is its nibble * 17 (palette entries 246-255 opaque), else 255.
Image decode_old_pixels(int width, int height, const uint8_t* data, std::size_t size, const uint8_t* palette,
                        const uint8_t* mask = nullptr, std::size_t mask_size = 0);

}  // namespace lithtech::dtx
