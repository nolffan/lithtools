// PS2 LITH_PS2.REZ reading and writing; docs/PS2REZ.md has the format and
// what LTIOREZ.IRX (the IOP module that reads it) requires of it.

#include "lithtech/ps2rez.hpp"
#include "lithtech/butes.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <set>

namespace lithtech::ps2rez {

// The compiled-in names (generated from core/data/ps2-names.txt).
extern const char kNamesText[];
extern const std::size_t kNamesTextSize;

namespace {

namespace fs = std::filesystem;

uint32_t rd32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
void wr32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v), p[1] = uint8_t(v >> 8), p[2] = uint8_t(v >> 16), p[3] = uint8_t(v >> 24);
}
uint32_t sectors_for(uint64_t bytes) { return uint32_t((bytes + kSector - 1) / kSector); }

std::vector<uint8_t> read_file_at(const std::string& path, uint64_t off, std::size_t n) {
    std::ifstream f(fs::u8path(path), std::ios::binary);
    if (!f) throw Error("can't open " + path);
    f.seekg(std::streamoff(off));
    std::vector<uint8_t> b(n);
    f.read(reinterpret_cast<char*>(b.data()), std::streamsize(n));
    b.resize(std::size_t(f.gcount()));
    return b;
}

uint64_t file_size_of(const std::string& path) {
    std::error_code ec;
    auto s = fs::file_size(fs::u8path(path), ec);
    return ec ? 0 : uint64_t(s);
}

bool header_ok(const uint8_t* h, uint64_t avail) {
    uint32_t ver = rd32(h), n = rd32(h + 4), w = rd32(h + 8), data = rd32(h + 12), nt = rd32(h + 16);
    if (ver != kVersion || n == 0 || n > kMaxEntries || w == 0 || nt == 0 || nt > 4096) return false;
    uint64_t stride = uint64_t(sectors_for(uint64_t(n) * 16)) * kSector;
    // tables follow the header's sector; data begins after them
    return uint64_t(data) * kSector == kSector + stride * nt && uint64_t(data) * kSector <= avail;
}

void split_lines(std::string_view text, const std::function<void(std::string)>& f) {
    std::size_t p = 0;
    while (p < text.size()) {
        std::size_t e = text.find('\n', p);
        if (e == std::string_view::npos) e = text.size();
        std::string line(text.substr(p, e - p));
        p = e + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        std::size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos || line[s] == '#') continue;
        f(line.substr(s));
    }
}

}  // namespace

uint32_t hash(std::string_view name, uint32_t weight) {
    uint32_t h = 0;
    for (char ch : name) {
        uint32_t c = uint8_t(std::toupper(static_cast<unsigned char>(ch)));
        if (c == '/') c = '\\';
        h = h + c + h * weight;
    }
    return h;
}

Names& Names::global() {
    static Names n = [] {
        Names x;
        split_lines(std::string_view(kNamesText, kNamesTextSize), [&](std::string s) { x.add(std::move(s)); });
        x.add(kArchiveNamesFile);
        return x;
    }();
    return n;
}

void Names::add(std::string name) {
    for (char& c : name)
        if (c == '/') c = '\\';
    names_.push_back(std::move(name));
    for (auto& [w, m] : by_weight_) m.emplace(hash(names_.back(), w), names_.size() - 1);
}

std::size_t Names::load_file(const std::string& path) {
    std::ifstream f(fs::u8path(path), std::ios::binary);
    if (!f) throw Error("can't open " + path);
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::size_t n = 0;
    split_lines(text, [&](std::string s) {
        // a "HASH<tab>name" line (the recovery script's lists) or just a name
        auto tab = s.find('\t');
        if (tab != std::string::npos) s = s.substr(tab + 1);
        add(std::move(s));
        ++n;
    });
    return n;
}

const std::string* Names::find(uint32_t h, uint32_t weight) const {
    auto it = by_weight_.find(weight);
    if (it == by_weight_.end()) {
        auto& m = by_weight_[weight];
        // the first name listed for a hash wins
        for (std::size_t i = 0; i < names_.size(); ++i) m.emplace(hash(names_[i], weight), i);
        it = by_weight_.find(weight);
    }
    auto f = it->second.find(h);
    return f == it->second.end() ? nullptr : &names_[f->second];
}

std::string guess_extension(const uint8_t* d, std::size_t n) {
    auto is = [&](std::size_t at, const char* s) { return n >= at + std::strlen(s) && !std::memcmp(d + at, s, std::strlen(s)); };
    if (n >= 8 && rd32(d + 4) == 0xFFFFFFFBu) return "dtx";      // DTX version -5
    if (is(0, "LithTech PSX Sound")) return "lps";
    if (is(0, "RIFF")) return "wav";
    if (is(0, "MZ")) return "dll";
    if (n >= 2 && d[0] == 0x0A && d[1] == 0x05) return "pcx";
    if (n >= 4 && rd32(d) == 2) return "ltb";                        // PS2 model
    if (n >= 4 && rd32(d) == 0x42) return "ltb";                     // PS2 world (version 66)
    if (is(0, "/*") || is(0, "\r\n")) return "txt";
    if (butes::looks_like(d, n)) return "but";   // needs the whole file
    return "bin";
}

std::optional<Location> locate(const std::string& path) {
    uint64_t fsize = file_size_of(path);
    if (fsize < kSector) return std::nullopt;
    auto pvd = read_file_at(path, 16ull * kSector, kSector);
    Location loc;
    if (pvd.size() == kSector && pvd[0] == 1 && !std::memcmp(pvd.data() + 1, "CD001", 5)) {
        uint32_t block = uint32_t(pvd[128] | pvd[129] << 8);
        if (block != kSector) throw Error(path + ": ISO image with " + std::to_string(block) + "-byte blocks");
        const uint8_t* root = pvd.data() + 156;
        uint32_t ext = rd32(root + 2), size = rd32(root + 10);
        if (size > (16u << 20)) throw Error(path + ": implausible ISO root directory");
        auto dir = read_file_at(path, uint64_t(ext) * kSector, size);
        std::size_t p = 0;
        while (p < dir.size()) {
            uint8_t len = dir[p];
            if (len == 0) {
                p = (p / kSector + 1) * kSector;
                continue;
            }
            if (p + len > dir.size() || len < 34) break;
            uint8_t nl = dir[p + 32];
            if (34u + nl > len) break;
            std::string name(reinterpret_cast<const char*>(&dir[p + 33]), nl);
            if (auto semi = name.find(';'); semi != std::string::npos) name.resize(semi);
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            if (name == "LITH_PS2.REZ" && !(dir[p + 25] & 2)) {
                loc.in_iso = true;
                loc.lsn = rd32(&dir[p + 2]);
                loc.offset = uint64_t(loc.lsn) * kSector;
                loc.size = rd32(&dir[p + 10]);
                loc.volume_sectors = rd32(pvd.data() + 80);
                break;
            }
            p += len;
        }
        if (!loc.in_iso) return std::nullopt;   // a disc image, but not one of these
    } else {
        loc.size = fsize;
    }
    auto h = read_file_at(path, loc.offset, 20);
    if (h.size() < 20 || !header_ok(h.data(), std::min<uint64_t>(loc.size, fsize - std::min(fsize, loc.offset))))
        return std::nullopt;
    return loc;
}

bool probe(const std::string& path) {
    try {
        return locate(path).has_value();
    } catch (const std::exception&) {
        return false;
    }
}

std::unique_ptr<Archive> Archive::open(const std::string& path) {
    auto loc = locate(path);
    if (!loc) throw Error(path + ": not a PS2 LITH_PS2.REZ (or a disc image holding one)");
    auto a = std::make_unique<Archive>();
    a->path = path;
    a->where = *loc;
    auto h = read_file_at(path, loc->offset, 20);
    a->header = {rd32(&h[0]), rd32(&h[4]), rd32(&h[8]), rd32(&h[12]), rd32(&h[16])};
    const uint32_t n = a->header.entries, stride = a->table_stride();
    auto raw = read_file_at(path, loc->offset + kSector, std::size_t(stride) * a->header.tables);
    if (raw.size() < std::size_t(stride) * a->header.tables) throw Error(path + ": the tables run past the end");
    a->tables.resize(a->header.tables);
    for (uint32_t t = 0; t < a->header.tables; ++t) {
        auto& tab = a->tables[t];
        tab.resize(n);
        const uint8_t* b = raw.data() + std::size_t(t) * stride;
        for (uint32_t i = 0; i < n; ++i) tab[i] = {rd32(b + 16 * i), rd32(b + 16 * i + 4), rd32(b + 16 * i + 8), rd32(b + 16 * i + 12)};
    }
    return a;
}

uint32_t Archive::table_stride() const { return sectors_for(uint64_t(header.entries) * 16) * kSector; }

uint32_t Archive::capacity() const { return std::min(kMaxEntries, table_stride() / 16); }

const Entry* Archive::find(uint32_t h) const {
    if (tables.empty()) return nullptr;
    for (auto& e : tables[0])
        if (e.hash == h) return &e;
    return nullptr;
}

std::vector<uint8_t> Archive::read_at(uint64_t offset, uint32_t size) const {
    auto b = read_file_at(path, offset, size);
    if (b.size() != size) throw Error(path + ": data past the end of the file");
    return b;
}

std::vector<uint8_t> Archive::read(const Entry& e) const { return read_at(file_offset(e), e.size); }

std::string Archive::name_of(const Entry& e) const {
    if (const std::string* n = Names::global().find(e.hash, header.weight)) return *n;
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08X", e.hash);
    std::string ext = "bin";
    if (e.size) {
        // small files whole, so compiled attribute files can be recognised
        auto head = read_at(file_offset(e), e.size <= (1u << 20) ? e.size : 32);
        ext = guess_extension(head.data(), head.size());
    }
    return std::string("_unnamed\\") + buf + "." + ext;
}

namespace {

// Streams src to dst.
void copy_file(const std::string& src, const std::string& dst) {
    std::ifstream in(fs::u8path(src), std::ios::binary);
    std::ofstream out(fs::u8path(dst), std::ios::binary | std::ios::trunc);
    if (!in || !out) throw Error("can't copy " + src + " to " + dst);
    std::vector<char> buf(8 << 20);
    while (in) {
        in.read(buf.data(), std::streamsize(buf.size()));
        out.write(buf.data(), in.gcount());
    }
    if (!out) throw Error("writing " + dst + " failed");
}

void write_at(std::fstream& f, uint64_t off, const void* p, std::size_t n) {
    f.seekp(std::streamoff(off));
    f.write(static_cast<const char*>(p), std::streamsize(n));
    if (!f) throw Error("write failed");
}

void zero_fill(std::fstream& f, uint64_t off, uint64_t n) {
    static const std::vector<char> z(64 << 10, 0);
    f.seekp(std::streamoff(off));
    while (n) {
        auto k = std::min<uint64_t>(n, z.size());
        f.write(z.data(), std::streamsize(k));
        n -= k;
    }
    if (!f) throw Error("write failed");
}

}  // namespace

Archive::SaveReport Archive::save(const std::string& out, const std::map<uint32_t, Change>& changes) {
    SaveReport rep;
    // Work out the new tables first (from the tables as they are: changes
    // read each other's original entries); nothing is written if they don't
    // fit.
    auto tabs = tables;
    const auto& orig = tables;
    std::set<uint32_t> present;
    for (auto& e : orig.empty() ? std::vector<Entry>{} : orig[0]) present.insert(e.hash);
    // where data can be appended, in sectors from the archive's first sector
    uint64_t fsize = file_size_of(path);
    uint32_t end = where.in_iso ? std::max<uint32_t>(where.volume_sectors, sectors_for(fsize)) - where.lsn
                                : sectors_for(fsize);
    for (auto& t : orig)
        for (auto& e : t) end = std::max(end, e.sector + e.sectors);

    auto entry_in = [](const std::vector<Entry>& t, uint32_t h) -> const Entry* {
        for (auto& e : t)
            if (e.hash == h) return &e;
        return nullptr;
    };
    auto set_entry = [](std::vector<Entry>& t, const Entry& ne) {
        for (auto& e : t)
            if (e.hash == ne.hash) {
                e = ne;
                return;
            }
        t.push_back(ne);
    };
    struct Write { uint32_t sector; std::shared_ptr<const std::vector<uint8_t>> data; uint32_t clear_sectors; };
    std::vector<Write> writes;
    for (auto& change : changes) {
        const uint32_t h = change.first;
        const Change& c = change.second;
        const bool exists = present.count(h) > 0;
        if (c.kind == Change::Kind::Remove) {
            if (!exists) continue;
            for (auto& t : tabs) t.erase(std::remove_if(t.begin(), t.end(), [h](const Entry& e) { return e.hash == h; }), t.end());
            present.erase(h);
            rep.removed++;
            continue;
        }
        if (c.kind == Change::Kind::Copy) {
            if (!entry_in(orig[0], c.from)) throw Error("no file to copy (hash " + std::to_string(c.from) + ")");
            for (std::size_t t = 0; t < tabs.size(); ++t) {
                Entry ne = *entry_in(orig[t], c.from);
                ne.hash = h;
                set_entry(tabs[t], ne);
            }
            if (!exists) rep.added++;
            present.insert(h);
            rep.copied++;
            continue;
        }
        const auto& d = *c.data;
        if (d.size() > 0xFFFFFFFFull) throw Error("file too big for the archive");
        const uint32_t size = uint32_t(d.size()), secs = sectors_for(d.size());
        if (!exists) {
            Entry ne{h, end, size, secs};
            if (size) writes.push_back({end, c.data, secs}), end += secs;
            for (auto& t : tabs) t.push_back(ne);
            present.insert(h);
            rep.added++;
            rep.appended++;
            continue;
        }
        // table 0's version and every table's identical copy of it
        const Entry e0 = *entry_in(orig[0], h);
        const auto bytes0 = read(e0);
        std::set<uint32_t> copies{e0.sector};
        std::map<uint32_t, bool> same;   // by sector
        uint32_t room = e0.sectors;
        for (auto& t : orig) {
            const Entry* e = entry_in(t, h);
            if (!e || e->sector == e0.sector) continue;
            auto it = same.find(e->sector);
            if (it == same.end()) it = same.emplace(e->sector, e->size == e0.size && read(*e) == bytes0).first;
            if (it->second) copies.insert(e->sector), room = std::min(room, e->sectors);
        }
        std::vector<std::size_t> which;   // tables whose entry is one of the copies
        for (std::size_t t = 0; t < tabs.size(); ++t) {
            const Entry* e = entry_in(orig[t], h);
            if (e && copies.count(e->sector)) which.push_back(t);
        }
        Entry ne{h, 0, size, secs};
        if (size && secs <= room) {
            for (uint32_t sct : copies) writes.push_back({sct, c.data, room});
            for (std::size_t t : which) {
                Entry x = *entry_in(orig[t], h);
                x.size = size, x.sectors = secs;
                set_entry(tabs[t], x);
            }
            rep.replaced_in_place++;
            continue;
        }
        ne.sector = end;
        if (size) writes.push_back({end, c.data, secs}), end += secs;
        for (std::size_t t : which) set_entry(tabs[t], ne);
        rep.appended++;
    }
    const uint32_t n = tabs.empty() ? 0 : uint32_t(tabs[0].size());
    for (auto& t : tabs)
        if (t.size() != n) throw Error("internal error: tables of different lengths");
    if (n > capacity())
        throw Error("the tables hold at most " + std::to_string(capacity()) + " files without moving the data (" +
                    std::to_string(n) + " wanted)");
    if (n == 0) throw Error("an archive needs at least one file");

    // Then the file.
    std::error_code ec;
    bool same_file = fs::exists(fs::u8path(out), ec) && fs::equivalent(fs::u8path(out), fs::u8path(path), ec);
    if (!same_file) copy_file(path, out);
    std::fstream f(fs::u8path(out), std::ios::binary | std::ios::in | std::ios::out);
    if (!f) throw Error("can't write " + out);
    for (auto& w : writes) {
        uint64_t off = where.offset + uint64_t(w.sector) * kSector;
        write_at(f, off, w.data->data(), w.data->size());
        // the rest of the space it has (old data, or the end padding) zeroed
        uint64_t room = uint64_t(w.clear_sectors) * kSector;
        if (room > w.data->size()) zero_fill(f, off + w.data->size(), room - w.data->size());
    }
    // header and tables
    std::vector<uint8_t> head(kSector, 0);
    wr32(&head[0], header.version), wr32(&head[4], n), wr32(&head[8], header.weight);
    wr32(&head[12], header.data_sector), wr32(&head[16], header.tables);
    write_at(f, where.offset, head.data(), head.size());
    const uint32_t stride = table_stride();   // unchanged: n fits the same sectors
    std::vector<uint8_t> tb(stride);
    for (uint32_t t = 0; t < tabs.size(); ++t) {
        std::fill(tb.begin(), tb.end(), 0);
        for (uint32_t i = 0; i < n; ++i) {
            const Entry& e = tabs[t][i];
            wr32(&tb[16 * i], e.hash), wr32(&tb[16 * i + 4], e.sector), wr32(&tb[16 * i + 8], e.size), wr32(&tb[16 * i + 12], e.sectors);
        }
        write_at(f, where.offset + kSector + uint64_t(t) * stride, tb.data(), tb.size());
    }
    // a disc image's volume grows to take appended data
    uint64_t new_len = std::max<uint64_t>(fsize, where.offset + uint64_t(end) * kSector);
    if (where.in_iso) {
        uint32_t vol = std::max<uint32_t>(where.volume_sectors, where.lsn + end);
        if (vol != where.volume_sectors) {
            uint8_t v[8];
            wr32(v, vol);
            v[4] = uint8_t(vol >> 24), v[5] = uint8_t(vol >> 16), v[6] = uint8_t(vol >> 8), v[7] = uint8_t(vol);
            write_at(f, 16ull * kSector + 80, v, 8);
            where.volume_sectors = vol;
        }
        new_len = std::max<uint64_t>(new_len, uint64_t(vol) * kSector);
    }
    f.close();
    if (file_size_of(out) < new_len) {
        fs::resize_file(fs::u8path(out), new_len, ec);
        if (ec) throw Error("can't extend " + out);
    }
    if (!where.in_iso) where.size = std::max<uint64_t>(where.size, uint64_t(end) * kSector);
    tables = std::move(tabs);
    header.entries = n;
    path = out;
    rep.end_sector = end;
    return rep;
}

void Archive::write_rebuilt(const std::string& out) const {
    std::ofstream f(fs::u8path(out), std::ios::binary | std::ios::trunc);
    if (!f) throw Error("can't write " + out);
    std::vector<uint8_t> head(kSector, 0);
    wr32(&head[0], header.version), wr32(&head[4], header.entries), wr32(&head[8], header.weight);
    wr32(&head[12], header.data_sector), wr32(&head[16], header.tables);
    f.write(reinterpret_cast<const char*>(head.data()), kSector);
    const uint32_t stride = table_stride();
    std::vector<uint8_t> tb(stride);
    for (auto& tab : tables) {
        std::fill(tb.begin(), tb.end(), 0);
        for (std::size_t i = 0; i < tab.size(); ++i) {
            const Entry& e = tab[i];
            wr32(&tb[16 * i], e.hash), wr32(&tb[16 * i + 4], e.sector), wr32(&tb[16 * i + 8], e.size), wr32(&tb[16 * i + 12], e.sectors);
        }
        f.write(reinterpret_cast<const char*>(tb.data()), stride);
    }
    // every extent once, in sector order, zeros between
    std::map<uint32_t, Entry> ext;
    for (auto& tab : tables)
        for (auto& e : tab)
            if (e.size) ext.emplace(e.sector, e);
    uint64_t pos = uint64_t(header.data_sector) * kSector;
    std::vector<char> z(kSector, 0);
    for (auto& [s, e] : ext) {
        uint64_t at = uint64_t(s) * kSector;
        if (at < pos) throw Error("extents overlap at sector " + std::to_string(s));
        while (pos < at) {
            auto k = std::min<uint64_t>(at - pos, z.size());
            f.write(z.data(), std::streamsize(k));
            pos += k;
        }
        auto d = read(e);
        f.write(reinterpret_cast<const char*>(d.data()), std::streamsize(d.size()));
        pos += d.size();
        uint64_t padded = uint64_t(e.sectors) * kSector;
        if (padded > d.size()) {
            f.write(z.data(), std::streamsize(padded - d.size()));
            pos += padded - d.size();
        }
    }
    while (pos < where.size) {
        auto k = std::min<uint64_t>(where.size - pos, z.size());
        f.write(z.data(), std::streamsize(k));
        pos += k;
    }
    if (!f) throw Error("writing " + out + " failed");
}

}  // namespace lithtech::ps2rez
