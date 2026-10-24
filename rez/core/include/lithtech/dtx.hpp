// LithTech .dtx textures, version -5 (every shipped file from NOLF to Tron
// 2.0): reading and decoding to RGBA. See plugins/blender/docs/DTX.md for
// the format and the evidence.

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

    Bpp bpp() const { return Bpp(extra[2] ? extra[2] : BPP_32); }
    int texture_group() const { return extra[0]; }
    int mipmaps_to_use() const { return extra[1]; }
    int non_s3tc_mip_offset() const { return extra[3]; }
    int ui_mip_offset() const { return extra[4]; }
    int priority() const { return extra[5]; }
    float detail_scale() const;
    int detail_angle() const;
    bool is_cubemap() const { return iflags & DTX_CUBEMAP; }
    const Section* find_section(const std::string& type) const;

    // One level (and cube face 0-5) as RGBA.
    Image decode(int mip = 0, int face = 0) const;
};

const char* bpp_name(Bpp b);
std::size_t image_size(Bpp bpp, int width, int height);

// lenient: keep a file that ends inside its pixels (two Tron prototypes).
Texture read(const std::vector<uint8_t>& data, bool lenient = true);
Texture read_file(const std::string& path, bool lenient = true);

// RGBA from one level's stored pixels.
Image decode_pixels(Bpp bpp, int width, int height, const uint8_t* data, std::size_t size,
                    const uint8_t* palette_bgra = nullptr, uint32_t iflags = 0);

}  // namespace lithtech::dtx
