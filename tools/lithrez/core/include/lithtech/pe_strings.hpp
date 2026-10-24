// String tables (RT_STRING) in Windows PE resource DLLs, such as a LithTech
// game's CRes.dll: reading them, and writing the DLL back with its resource
// section rebuilt. Standard C++17, no Windows headers; PE32 and PE32+.
//
//   auto img = lithtech::pe::Image::parse(bytes);
//   auto table = img.strings(img.string_languages().front());
//   table[123] = u"Hello";
//   img.set_strings(lang, table);
//   std::vector<uint8_t> out = img.write();
//
// The resource tree is kept whole (every type, name, language, code page
// and byte); only the RT_STRING blocks are regenerated, and only those whose
// strings changed. The section is laid out as Microsoft's linker lays it
// out (directories breadth first, then data entries, then names, then the
// data, each 8-byte aligned, in the original order), so an unedited DLL
// writes back byte for byte.

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace lithtech::pe {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

constexpr uint16_t RT_STRING = 6;

// A resource directory entry's name: an ID, or a string.
struct Name {
    bool is_id = true;
    uint16_t id = 0;
    std::u16string str;
    bool operator==(const Name& o) const { return is_id == o.is_id && (is_id ? id == o.id : str == o.str); }
};

// A directory (children) or a leaf (data).
struct Node {
    Name name;
    bool is_dir = true;
    // directory
    uint32_t characteristics = 0, timestamp = 0;
    uint16_t major = 0, minor = 0;
    std::vector<Node> children;
    // leaf
    std::vector<uint8_t> data;
    uint32_t codepage = 0, reserved = 0;
    // The data's place in the original section, to keep the order on write.
    uint64_t order = UINT64_MAX;

    Node* find(const Name& n);
    const Node* find(const Name& n) const;
};

// id -> string. Ids run 0-65535; block N of RT_STRING holds ids
// (N-1)*16 .. (N-1)*16+15. An empty string is no string (LoadString can't
// tell them apart).
using StringTable = std::map<uint32_t, std::u16string>;

struct Section {
    std::string name;
    uint32_t virtual_size = 0, virtual_address = 0, raw_size = 0, raw_pointer = 0, characteristics = 0;
};

struct WriteInfo {
    // "in place", "grown (last section)", "moved .reloc", "new section"
    std::string placement;
    uint32_t rsrc_rva = 0, rsrc_size = 0;
    std::vector<std::string> warnings;
};

class Image {
public:
    // Throws Error if the bytes aren't a PE image or its resources can't be
    // read.
    static Image parse(std::vector<uint8_t> bytes);

    bool pe32_plus() const { return pe32_plus_; }
    const std::vector<Section>& sections() const { return sections_; }
    bool has_resources() const { return has_rsrc_; }

    Node& resources() { return root_; }
    const Node& resources() const { return root_; }

    // The languages that have RT_STRING blocks, ascending.
    std::vector<uint16_t> string_languages() const;
    StringTable strings(uint16_t language) const;
    // Replace the language's strings (a language not there yet is added).
    void set_strings(uint16_t language, const StringTable& table);

    // The image with its resource section rebuilt. Throws Error if it can't
    // be placed (no room for a new section header and nowhere to grow).
    // allow_moving_reloc: when the resources must grow and only .reloc
    // follows them, move .reloc up (the default) rather than add a section.
    std::vector<uint8_t> write(WriteInfo* info = nullptr, bool allow_moving_reloc = true) const;

    // The resource section's bytes laid out for the given RVA.
    std::vector<uint8_t> layout(uint32_t base_rva) const;
    // Whether layout() gives back the stored section byte for byte (true
    // for Microsoft-linked DLLs such as every CRes.dll shipped).
    bool layout_matches_original() const;
    // Whether set_strings changed anything; write() of an unmodified image
    // returns the original bytes.
    bool modified() const { return modified_; }

private:
    std::vector<uint8_t> bytes_;
    bool pe32_plus_ = false;
    bool has_rsrc_ = false;
    bool modified_ = false;
    uint32_t pe_ = 0, opt_ = 0, sect_table_ = 0;
    uint16_t nsections_ = 0;
    uint32_t file_align_ = 512, sect_align_ = 4096, size_of_headers_ = 0;
    uint32_t num_dirs_ = 0, dirs_ = 0;
    uint32_t rsrc_rva_ = 0, rsrc_size_ = 0;
    int rsrc_section_ = -1;
    std::vector<Section> sections_;
    Node root_;

    uint32_t rva_to_offset(uint32_t rva, uint32_t size) const;
};

std::string to_utf8(const std::u16string& s);
std::u16string from_utf8(const std::string& s);
// The PE checksum of an image whose checksum field is at checksum_offset.
uint32_t checksum(const std::vector<uint8_t>& image, uint32_t checksum_offset);
// "English (United States)" etc. for common LANGIDs, else "0x0409".
std::string language_name(uint16_t langid);

}  // namespace lithtech::pe
