// The PS2 resource archive: LITH_PS2.REZ on The Operative: No One Lives
// Forever's PS2 discs. A different format from the PC's RezMgr files: 2048-
// byte sectors, and files known only by a hash of their name, through 506
// tables. Table 0 has every file; the other 505, five per level, name the
// same files but point many of them elsewhere: at copies laid out next to
// that level's other files, or at variants (PAL screen layouts, other
// languages' voice banks). See docs/PS2REZ.md for the layout and the
// evidence (the IOP module LTIOREZ.IRX that reads it).
//
// rez::Archive::open reads these too, as a tree named from a dictionary of
// recovered names (core/data/ps2-names.txt); this is the layer underneath.

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lithtech::ps2rez {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline constexpr uint32_t kSector = 2048;
inline constexpr uint32_t kVersion = 3;          // RezTOCHeader.m_Version, checked by LTIOREZ
inline constexpr uint32_t kMaxEntries = 6500;    // LTIOREZ refuses more (its table buffer)
inline constexpr uint32_t kHashWeight = 10;      // m_HashWeight on both discs

// RezTOCHeader (LTIOREZ's names), the first 20 bytes of the archive.
struct Header {
    uint32_t version = kVersion;
    uint32_t entries = 0;       // m_NumFiles: entries in every table
    uint32_t weight = kHashWeight;  // m_HashWeight
    uint32_t data_sector = 0;   // m_Offset: first sector after the tables
    uint32_t tables = 0;        // m_NumTOC
};

// RezTOCEntry: 16 bytes.
struct Entry {
    uint32_t hash = 0;      // m_nHash
    uint32_t sector = 0;    // m_nOffsetSectors, from the archive's first sector
    uint32_t size = 0;      // m_nFileSize, bytes
    uint32_t sectors = 0;   // m_nFileSizeSectors = ceil(size / 2048)
};

// h = h + c + h * weight over the name, upper-cased, '/' as '\\'
// (rezCalcHash).
uint32_t hash(std::string_view name, uint32_t weight = kHashWeight);

// File names for hashes: the compiled-in list, plus any added.
class Names {
public:
    // Names an archive was given that the list doesn't have: lithrez keeps
    // them in the archive itself, in this file, and reads them back.
    static constexpr const char* kArchiveNamesFile = "LITHTOOLS\\NAMES.TXT";
    // The recovered names (core/data/ps2-names.txt) and whatever has been
    // added to it (e.g. lithrez --names).
    static Names& global();
    void add(std::string name);
    // One name per line; '#' starts a comment. Returns the number read.
    std::size_t load_file(const std::string& path);
    const std::string* find(uint32_t h, uint32_t weight = kHashWeight) const;
    std::size_t size() const { return names_.size(); }

private:
    std::vector<std::string> names_;
    mutable std::map<uint32_t, std::unordered_map<uint32_t, std::size_t>> by_weight_;
};

// A file extension guessed from the first bytes, for unnamed entries.
std::string guess_extension(const uint8_t* data, std::size_t size);

// Where the archive is: a file of its own, or LITH_PS2.REZ inside an
// ISO9660 disc image.
struct Location {
    bool in_iso = false;
    uint64_t offset = 0;        // the archive's first byte in the file
    uint64_t size = 0;          // the archive's size (its ISO directory record's)
    uint32_t lsn = 0;           // in an ISO: its first sector
    uint32_t volume_sectors = 0;  // in an ISO: the primary volume descriptor's volume space size
};

// nullopt if path is neither an ISO image holding LITH_PS2.REZ nor a file
// that starts like one.
std::optional<Location> locate(const std::string& path);
bool probe(const std::string& path);

class Archive {
public:
    static std::unique_ptr<Archive> open(const std::string& path);

    std::string path;
    Location where;
    Header header;
    std::vector<std::vector<Entry>> tables;   // [table][entry]

    // Table bytes rounded up to whole sectors (LTIOREZ reads them so).
    uint32_t table_stride() const;
    // How many entries fit each table without moving the tables or data.
    uint32_t capacity() const;
    // Table 0's entry for a hash, or nullptr.
    const Entry* find(uint32_t h) const;
    // Where an entry's bytes are in the file.
    uint64_t file_offset(const Entry& e) const { return where.offset + uint64_t(e.sector) * kSector; }
    std::vector<uint8_t> read(const Entry& e) const;
    std::vector<uint8_t> read_at(uint64_t offset, uint32_t size) const;
    // The name of an entry: the dictionary's, else "_unnamed\\<HASH>.<ext>".
    std::string name_of(const Entry& e) const;

    // A change to save: new bytes for a name (replacing it, or adding it),
    // another name's entries under this one (renames, moves and copies keep
    // their data where it is, each table's own, variants and all), or
    // removing a name.
    struct Change {
        enum class Kind { Data, Copy, Remove } kind = Kind::Data;
        std::shared_ptr<const std::vector<uint8_t>> data;   // Data
        uint32_t from = 0;                                   // Copy: the name (hash) whose entries to take
    };
    struct SaveReport {
        int replaced_in_place = 0, appended = 0, added = 0, removed = 0, copied = 0;
        uint32_t end_sector = 0;   // where the archive's data now ends (from its first sector)
    };
    // Writes the archive with the changes (keyed by hash) to out: the source
    // file (archive or disc image) is copied, then patched. Replacing a file
    // replaces table 0's version and every table's identical copy of it
    // (variants stay): over those copies when it fits them, else once after
    // the end (of the archive, or in an image after the last sector of the
    // volume, which grows), with those entries pointing there. New names go
    // at the end of every table; removed ones come out of all of them. out
    // may be the source file itself (patched in place). Afterwards this
    // object describes the result.
    SaveReport save(const std::string& out, const std::map<uint32_t, Change>& changes);

    // Writes the archive alone (not an image), rebuilt from the tables:
    // header, tables, each extent at its sector, zeros elsewhere. Shipped
    // archives come out byte-identical (docs/PS2REZ.md).
    void write_rebuilt(const std::string& out) const;
};

}  // namespace lithtech::ps2rez
