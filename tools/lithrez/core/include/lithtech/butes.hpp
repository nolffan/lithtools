// The PS2's compiled attribute files (.but): the binary form of a ButeMgr
// text file ([Tag] / Key = value), as the PS2 version loads its attributes
// and asset lists. Tag and key names are stored only as hashes
// (h = h * 31 + c, case-sensitive); a list of the names (core/data/
// ps2-bute-names.txt, from the PS2's and the PC games' text attribute
// files) names every one in the shipped files. See docs/PS2REZ.md.

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lithtech::butes {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

enum Type : uint16_t {
    String = 1, Int = 2, Float = 3, Vector = 4, Point = 5, Bool = 6, Range = 7, Rect = 8,
};

struct Key {
    uint32_t hash = 0;
    uint16_t type = 0;
    std::vector<uint8_t> value;   // as stored (strings with their NUL and padding)
};

struct Tag {
    uint32_t hash = 0;
    std::vector<Key> keys;
};

uint32_t hash(std::string_view name);

// Whether the bytes parse as a compiled attribute file, exactly to the end.
bool looks_like(const uint8_t* data, std::size_t size);
std::vector<Tag> read(const uint8_t* data, std::size_t size);

// A name for a hash from the compiled-in list, or "#XXXXXXXX".
std::string name_of(uint32_t hash);
// The file as ButeMgr text: [Tag] then Key = value lines.
std::string to_text(const std::vector<Tag>& tags);

}  // namespace lithtech::butes
