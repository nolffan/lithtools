// PE resource string tables; the layout rules are described in
// pe_strings.hpp, the placement rules in Image::write.

#include "lithtech/pe_strings.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <set>

namespace lithtech::pe {

namespace {

uint16_t rd16(const std::vector<uint8_t>& b, std::size_t o) {
    if (o + 2 > b.size()) throw Error("truncated PE image");
    return uint16_t(b[o] | b[o + 1] << 8);
}
uint32_t rd32(const std::vector<uint8_t>& b, std::size_t o) {
    if (o + 4 > b.size()) throw Error("truncated PE image");
    return uint32_t(b[o]) | uint32_t(b[o + 1]) << 8 | uint32_t(b[o + 2]) << 16 | uint32_t(b[o + 3]) << 24;
}
void wr16(std::vector<uint8_t>& b, std::size_t o, uint16_t v) {
    b[o] = uint8_t(v);
    b[o + 1] = uint8_t(v >> 8);
}
void wr32(std::vector<uint8_t>& b, std::size_t o, uint32_t v) {
    for (int i = 0; i < 4; ++i) b[o + i] = uint8_t(v >> (8 * i));
}
uint32_t align(uint32_t v, uint32_t a) { return a ? (v + a - 1) / a * a : v; }

char16_t up16(char16_t c) { return (c >= u'a' && c <= u'z') ? char16_t(c - 32) : c; }

// Windows keeps named entries before ID entries, names in upper-cased
// order, IDs ascending (the loader binary-searches them).
bool entry_less(const Name& a, const Name& b) {
    if (a.is_id != b.is_id) return !a.is_id;
    if (a.is_id) return a.id < b.id;
    std::size_t n = std::min(a.str.size(), b.str.size());
    for (std::size_t i = 0; i < n; ++i)
        if (up16(a.str[i]) != up16(b.str[i])) return up16(a.str[i]) < up16(b.str[i]);
    return a.str.size() < b.str.size();
}

Name id_name(uint16_t id) {
    Name n;
    n.id = id;
    return n;
}

Node& child_sorted(Node& dir, const Name& n) {
    if (Node* f = dir.find(n)) return *f;
    Node c;
    c.name = n;
    auto it = std::lower_bound(dir.children.begin(), dir.children.end(), c,
                               [](const Node& x, const Node& y) { return entry_less(x.name, y.name); });
    return *dir.children.insert(it, std::move(c));
}

// The 16 strings of an RT_STRING block (missing ones empty).
std::vector<std::u16string> decode_block(const std::vector<uint8_t>& d) {
    std::vector<std::u16string> out(16);
    std::size_t o = 0;
    for (int i = 0; i < 16 && o + 2 <= d.size(); ++i) {
        uint16_t n = uint16_t(d[o] | d[o + 1] << 8);
        o += 2;
        std::size_t take = std::min<std::size_t>(n, (d.size() - o) / 2);
        for (std::size_t k = 0; k < take; ++k) out[i].push_back(char16_t(d[o + 2 * k] | d[o + 2 * k + 1] << 8));
        o += 2 * std::size_t(n);
        if (o > d.size()) break;
    }
    return out;
}

std::vector<uint8_t> encode_block(const std::vector<std::u16string>& s) {
    std::vector<uint8_t> d;
    for (int i = 0; i < 16; ++i) {
        const std::u16string& x = s[std::size_t(i)];
        std::size_t n = std::min<std::size_t>(x.size(), 0xFFFF);
        d.push_back(uint8_t(n));
        d.push_back(uint8_t(n >> 8));
        for (std::size_t k = 0; k < n; ++k) {
            d.push_back(uint8_t(x[k]));
            d.push_back(uint8_t(x[k] >> 8));
        }
    }
    return d;
}

}  // namespace

Node* Node::find(const Name& n) {
    for (auto& c : children)
        if (c.name == n) return &c;
    return nullptr;
}
const Node* Node::find(const Name& n) const {
    for (auto& c : children)
        if (c.name == n) return &c;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Parsing

uint32_t Image::rva_to_offset(uint32_t rva, uint32_t size) const {
    for (auto& s : sections_) {
        uint32_t vsz = std::max(s.virtual_size, s.raw_size);
        if (rva >= s.virtual_address && uint64_t(rva) + size <= uint64_t(s.virtual_address) + vsz) {
            uint64_t off = uint64_t(s.raw_pointer) + (rva - s.virtual_address);
            if (rva - s.virtual_address + uint64_t(size) > s.raw_size) throw Error("resource data outside the file");
            if (off + size > bytes_.size()) throw Error("resource data past the end of the file");
            return uint32_t(off);
        }
    }
    if (uint64_t(rva) + size <= size_of_headers_ && uint64_t(rva) + size <= bytes_.size()) return rva;
    throw Error("RVA outside every section");
}

Image Image::parse(std::vector<uint8_t> bytes) {
    Image im;
    im.bytes_ = std::move(bytes);
    const auto& b = im.bytes_;
    if (b.size() < 64 || b[0] != 'M' || b[1] != 'Z') throw Error("not a PE image (no MZ header)");
    im.pe_ = rd32(b, 0x3c);
    if (uint64_t(im.pe_) + 24 > b.size() || std::memcmp(&b[im.pe_], "PE\0\0", 4) != 0)
        throw Error("not a PE image (no PE signature)");
    im.nsections_ = rd16(b, im.pe_ + 6);
    uint16_t optsize = rd16(b, im.pe_ + 20);
    im.opt_ = im.pe_ + 24;
    uint16_t magic = rd16(b, im.opt_);
    if (magic == 0x20b) im.pe32_plus_ = true;
    else if (magic != 0x10b) throw Error("unknown optional header magic");
    im.sect_align_ = rd32(b, im.opt_ + 32);
    im.file_align_ = rd32(b, im.opt_ + 36);
    im.size_of_headers_ = rd32(b, im.opt_ + 60);
    uint32_t ndirs_off = im.opt_ + (im.pe32_plus_ ? 108 : 92);
    im.num_dirs_ = rd32(b, ndirs_off);
    im.dirs_ = ndirs_off + 4;
    im.sect_table_ = im.opt_ + optsize;
    if (uint64_t(im.sect_table_) + 40ull * im.nsections_ > b.size()) throw Error("section table past the end of the file");
    for (uint16_t i = 0; i < im.nsections_; ++i) {
        uint32_t o = im.sect_table_ + 40u * i;
        Section s;
        char nm[9] = {};
        std::memcpy(nm, &b[o], 8);
        s.name = nm;
        s.virtual_size = rd32(b, o + 8);
        s.virtual_address = rd32(b, o + 12);
        s.raw_size = rd32(b, o + 16);
        s.raw_pointer = rd32(b, o + 20);
        s.characteristics = rd32(b, o + 36);
        im.sections_.push_back(s);
    }
    if (im.num_dirs_ > 2) {
        im.rsrc_rva_ = rd32(b, im.dirs_ + 16);
        im.rsrc_size_ = rd32(b, im.dirs_ + 20);
    }
    im.root_.is_dir = true;
    if (!im.rsrc_rva_ || !im.rsrc_size_) return im;
    for (std::size_t i = 0; i < im.sections_.size(); ++i) {
        auto& s = im.sections_[i];
        if (im.rsrc_rva_ >= s.virtual_address && im.rsrc_rva_ < s.virtual_address + std::max(s.virtual_size, s.raw_size))
            im.rsrc_section_ = int(i);
    }
    if (im.rsrc_section_ < 0) throw Error("resource directory outside every section");
    im.has_rsrc_ = true;

    // The section's bytes; data entries may in principle point elsewhere.
    const Section& rs = im.sections_[std::size_t(im.rsrc_section_)];
    uint32_t avail = rs.virtual_address + rs.raw_size - im.rsrc_rva_;
    if (rs.virtual_address + rs.raw_size <= im.rsrc_rva_) avail = 0;
    uint32_t base_off = uint32_t(rs.raw_pointer + (im.rsrc_rva_ - rs.virtual_address));
    uint32_t limit = std::min<uint32_t>(avail, uint32_t(std::min<uint64_t>(b.size() - std::min<uint64_t>(b.size(), base_off), UINT32_MAX)));

    std::set<uint32_t> seen;
    uint64_t data_seq = 0;
    std::vector<std::pair<uint32_t, Node*>> leaves;  // for ordering
    struct Walk {
        Image& im;
        const std::vector<uint8_t>& b;
        uint32_t base_off, limit;
        std::set<uint32_t>& seen;
        std::vector<std::pair<uint32_t, Node*>>& leaves;
        void need(uint32_t off, uint32_t n) {
            if (uint64_t(off) + n > limit) throw Error("resource directory runs past its section");
        }
        void dir(Node& n, uint32_t off, int depth) {
            if (depth > 8) throw Error("resource tree too deep");
            if (!seen.insert(off).second) throw Error("resource directory loop");
            need(off, 16);
            uint32_t o = base_off + off;
            n.is_dir = true;
            n.characteristics = rd32(b, o);
            n.timestamp = rd32(b, o + 4);
            n.major = rd16(b, o + 8);
            n.minor = rd16(b, o + 10);
            uint32_t count = uint32_t(rd16(b, o + 12)) + rd16(b, o + 14);
            need(off + 16, count * 8);
            n.children.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t eo = o + 16 + 8 * i;
                uint32_t nm = rd32(b, eo), od = rd32(b, eo + 4);
                Node c;
                if (nm & 0x80000000u) {
                    uint32_t so = nm & 0x7fffffffu;
                    need(so, 2);
                    uint16_t len = rd16(b, base_off + so);
                    need(so + 2, 2u * len);
                    c.name.is_id = false;
                    for (uint16_t k = 0; k < len; ++k) c.name.str.push_back(char16_t(rd16(b, base_off + so + 2 + 2u * k)));
                } else {
                    c.name.id = uint16_t(nm);
                }
                n.children.push_back(std::move(c));
                Node& cr = n.children.back();
                if (od & 0x80000000u) {
                    dir(cr, od & 0x7fffffffu, depth + 1);
                } else {
                    need(od, 16);
                    uint32_t de = base_off + od;
                    uint32_t rva = rd32(b, de), size = rd32(b, de + 4);
                    cr.is_dir = false;
                    cr.codepage = rd32(b, de + 8);
                    cr.reserved = rd32(b, de + 12);
                    uint32_t fo = im.rva_to_offset(rva, size);
                    cr.data.assign(b.begin() + fo, b.begin() + fo + size);
                    leaves.push_back({rva, &cr});
                }
            }
        }
    };
    Walk w{im, b, base_off, limit, seen, leaves};
    w.dir(im.root_, 0, 0);
    // Pointers into vectors are stable now (the tree is complete).
    std::stable_sort(leaves.begin(), leaves.end(), [](auto& x, auto& y) { return x.first < y.first; });
    for (auto& [rva, n] : leaves) n->order = data_seq++;
    return im;
}

// ---------------------------------------------------------------------------
// Strings

std::vector<uint16_t> Image::string_languages() const {
    std::set<uint16_t> langs;
    if (const Node* t = root_.find(id_name(RT_STRING)))
        for (auto& blk : t->children)
            for (auto& l : blk.children)
                if (l.name.is_id && !l.is_dir) langs.insert(l.name.id);
    return {langs.begin(), langs.end()};
}

StringTable Image::strings(uint16_t lang) const {
    StringTable t;
    const Node* type = root_.find(id_name(RT_STRING));
    if (!type) return t;
    for (auto& blk : type->children) {
        if (!blk.name.is_id || blk.name.id == 0) continue;
        const Node* l = blk.find(id_name(lang));
        if (!l || l->is_dir) continue;
        auto s = decode_block(l->data);
        for (int i = 0; i < 16; ++i)
            if (!s[std::size_t(i)].empty()) t[(uint32_t(blk.name.id) - 1) * 16 + uint32_t(i)] = s[std::size_t(i)];
    }
    return t;
}

void Image::set_strings(uint16_t lang, const StringTable& table) {
    std::map<uint16_t, std::vector<std::u16string>> blocks;
    for (auto& [id, s] : table) {
        if (id > 0xFFFF || s.empty()) continue;
        auto& v = blocks[uint16_t(id / 16 + 1)];
        if (v.empty()) v.resize(16);
        v[id % 16] = s;
    }
    Node& type = child_sorted(root_, id_name(RT_STRING));
    type.is_dir = true;
    // Drop or rewrite existing blocks of this language.
    for (auto it = type.children.begin(); it != type.children.end();) {
        Node& blk = *it;
        if (blk.name.is_id) {
            auto want = blocks.find(blk.name.id);
            for (auto lt = blk.children.begin(); lt != blk.children.end();) {
                if (lt->name.is_id && lt->name.id == lang && !lt->is_dir) {
                    if (want == blocks.end()) {
                        lt = blk.children.erase(lt);
                        modified_ = true;
                        continue;
                    }
                    if (decode_block(lt->data) != want->second) {
                        lt->data = encode_block(want->second);
                        modified_ = true;
                    }
                }
                ++lt;
            }
        }
        if (blk.children.empty()) it = type.children.erase(it);
        else ++it;
    }
    // New blocks.
    for (auto& [id, strs] : blocks) {
        Node& blk = child_sorted(type, id_name(id));
        blk.is_dir = true;
        if (!blk.find(id_name(lang))) {
            Node& l = child_sorted(blk, id_name(lang));
            l.is_dir = false;
            l.data = encode_block(strs);
            modified_ = true;
        }
    }
    if (type.children.empty())
        root_.children.erase(std::remove_if(root_.children.begin(), root_.children.end(),
                                            [](const Node& n) { return n.name.is_id && n.name.id == RT_STRING; }),
                             root_.children.end());
    has_rsrc_ = has_rsrc_ || !root_.children.empty();
}

// ---------------------------------------------------------------------------
// Layout

std::vector<uint8_t> Image::layout(uint32_t base) const {
    // Directories breadth first.
    std::vector<const Node*> dirs;
    std::deque<const Node*> q{&root_};
    while (!q.empty()) {
        const Node* n = q.front();
        q.pop_front();
        dirs.push_back(n);
        for (auto& c : n->children)
            if (c.is_dir) q.push_back(&c);
    }
    std::vector<const Node*> leaves, named;
    for (const Node* d : dirs)
        for (auto& c : d->children) {
            if (!c.is_dir) leaves.push_back(&c);
            if (!c.name.is_id) named.push_back(&c);
        }
    std::map<const Node*, uint32_t> dir_off, entry_off, name_off, data_off;
    uint32_t pos = 0;
    for (const Node* d : dirs) {
        dir_off[d] = pos;
        pos += 16 + 8 * uint32_t(d->children.size());
    }
    for (const Node* l : leaves) {
        entry_off[l] = pos;
        pos += 16;
    }
    // One copy of each distinct name string.
    std::map<std::u16string, uint32_t> strings;
    for (const Node* n : named) {
        auto it = strings.find(n->name.str);
        if (it == strings.end()) {
            strings[n->name.str] = pos;
            name_off[n] = pos;
            pos += 2 + 2 * uint32_t(n->name.str.size());
        } else {
            name_off[n] = it->second;
        }
    }
    std::vector<const Node*> by_order = leaves;
    std::stable_sort(by_order.begin(), by_order.end(), [](const Node* a, const Node* b) { return a->order < b->order; });
    // The linker starts the data on a 16-byte boundary and puts each block
    // on an 8-byte one.
    pos = align(pos, 16);
    for (const Node* l : by_order) {
        pos = align(pos, 8);
        data_off[l] = pos;
        pos += uint32_t(l->data.size());
    }
    pos = align(pos, 8);

    std::vector<uint8_t> out(pos, 0);
    for (const Node* d : dirs) {
        uint32_t o = dir_off[d];
        uint16_t nnamed = 0, nid = 0;
        for (auto& c : d->children) (c.name.is_id ? nid : nnamed)++;
        wr32(out, o, d->characteristics);
        wr32(out, o + 4, d->timestamp);
        wr16(out, o + 8, d->major);
        wr16(out, o + 10, d->minor);
        wr16(out, o + 12, nnamed);
        wr16(out, o + 14, nid);
        uint32_t e = o + 16;
        for (auto& c : d->children) {
            wr32(out, e, c.name.is_id ? c.name.id : (0x80000000u | name_off[&c]));
            wr32(out, e + 4, c.is_dir ? (0x80000000u | dir_off[&c]) : entry_off[&c]);
            e += 8;
        }
    }
    for (const Node* l : leaves) {
        uint32_t o = entry_off[l];
        wr32(out, o, base + data_off[l]);
        wr32(out, o + 4, uint32_t(l->data.size()));
        wr32(out, o + 8, l->codepage);
        wr32(out, o + 12, l->reserved);
        std::copy(l->data.begin(), l->data.end(), out.begin() + data_off[l]);
    }
    for (auto& [s, o] : strings) {
        wr16(out, o, uint16_t(s.size()));
        for (std::size_t k = 0; k < s.size(); ++k) wr16(out, o + 2 + 2 * uint32_t(k), uint16_t(s[k]));
    }
    return out;
}

// ---------------------------------------------------------------------------
// Writing

bool Image::layout_matches_original() const {
    if (!has_rsrc_) return true;
    auto rs = layout(rsrc_rva_);
    uint32_t fo;
    try {
        fo = rva_to_offset(rsrc_rva_, uint32_t(rs.size()));
    } catch (const Error&) {
        return false;
    }
    if (!std::equal(rs.begin(), rs.end(), bytes_.begin() + fo)) return false;
    return rs.size() == rsrc_size_ ||
           (rs.size() < rsrc_size_ && fo + rsrc_size_ <= bytes_.size() &&
            std::all_of(bytes_.begin() + fo + rs.size(), bytes_.begin() + fo + rsrc_size_, [](uint8_t c) { return c == 0; }));
}

uint32_t checksum(const std::vector<uint8_t>& b, uint32_t ck_off) {
    uint64_t sum = 0;
    std::size_t n = b.size();
    for (std::size_t i = 0; i < n; i += 2) {
        if (i == ck_off || i == ck_off + 2) continue;
        uint32_t w = b[i] | (i + 1 < n ? b[i + 1] << 8 : 0);
        sum += w;
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = (sum & 0xffff) + (sum >> 16);
    return uint32_t(sum) + uint32_t(n);
}

std::vector<uint8_t> Image::write(WriteInfo* info, bool allow_moving_reloc) const {
    WriteInfo local;
    WriteInfo& wi = info ? *info : local;
    if (!modified_) {
        wi.placement = "unchanged";
        wi.rsrc_rva = rsrc_rva_;
        wi.rsrc_size = rsrc_size_;
        return bytes_;
    }
    std::vector<uint8_t> out = bytes_;
    if (num_dirs_ < 3) throw Error("the image has no resource directory slot");
    uint32_t ck_off = opt_ + 64;
    bool had_checksum = rd32(bytes_, ck_off) != 0;
    std::vector<Section> secs = sections_;

    auto put_section = [&](std::size_t i) {
        uint32_t o = sect_table_ + 40u * uint32_t(i);
        const Section& s = secs[i];
        char nm[8] = {};
        std::memcpy(nm, s.name.data(), std::min<std::size_t>(8, s.name.size()));
        std::memcpy(&out[o], nm, 8);
        wr32(out, o + 8, s.virtual_size);
        wr32(out, o + 12, s.virtual_address);
        wr32(out, o + 16, s.raw_size);
        wr32(out, o + 20, s.raw_pointer);
        wr32(out, o + 36, s.characteristics);
    };
    auto image_end = [&]() {
        uint32_t end = 0;
        for (auto& s : secs) end = std::max(end, align(s.virtual_address + std::max(s.virtual_size, 1u), sect_align_));
        return end;
    };
    auto raw_end = [&]() {
        uint32_t end = size_of_headers_;
        for (auto& s : secs)
            if (s.raw_size) end = std::max(end, s.raw_pointer + s.raw_size);
        return end;
    };

    // Overlay (bytes after the last section's raw data) is kept at the end.
    // A certificate there would no longer match: it is dropped.
    uint32_t sec_dir_off = dirs_ + 8 * 4;
    if (num_dirs_ > 4 && rd32(bytes_, sec_dir_off) != 0) {
        uint32_t cert = rd32(bytes_, sec_dir_off);
        wr32(out, sec_dir_off, 0);
        wr32(out, sec_dir_off + 4, 0);
        if (cert < out.size()) out.resize(cert);
        wi.warnings.push_back("the image's digital signature was removed (it can't survive the edit)");
    }

    std::vector<uint8_t> rs;
    auto fits_in_place = [&](uint32_t rva, std::size_t sec, uint32_t& limit) {
        const Section& s = secs[sec];
        uint32_t vlimit = UINT32_MAX;
        for (auto& o : secs)
            if (o.virtual_address > s.virtual_address) vlimit = std::min(vlimit, o.virtual_address);
        uint32_t rawlimit = s.virtual_address + s.raw_size;
        limit = std::min(vlimit, rawlimit) - rva;
        return true;
    };

    if (has_rsrc_ && rsrc_section_ >= 0) {
        std::size_t si = std::size_t(rsrc_section_);
        uint32_t limit = 0;
        fits_in_place(rsrc_rva_, si, limit);
        rs = layout(rsrc_rva_);
        if (rs.size() <= limit) {
            Section& s = secs[si];
            uint32_t fo = s.raw_pointer + (rsrc_rva_ - s.virtual_address);
            uint32_t old_end = fo + std::max<uint32_t>(rsrc_size_, uint32_t(rs.size()));
            old_end = std::min<uint32_t>(old_end, s.raw_pointer + s.raw_size);
            std::fill(out.begin() + fo, out.begin() + old_end, 0);
            std::copy(rs.begin(), rs.end(), out.begin() + fo);
            s.virtual_size = std::max<uint32_t>(s.virtual_size, rsrc_rva_ - s.virtual_address + uint32_t(rs.size()));
            put_section(si);
            // An unchanged tree keeps the stored size (some have zero
            // padding counted in it).
            uint32_t size = uint32_t(rs.size());
            if (size < rsrc_size_ && fo + rsrc_size_ <= bytes_.size() &&
                std::equal(rs.begin(), rs.end(), bytes_.begin() + fo) &&
                std::all_of(bytes_.begin() + fo + size, bytes_.begin() + fo + rsrc_size_, [](uint8_t c) { return c == 0; }))
                size = rsrc_size_;
            wr32(out, dirs_ + 16, rsrc_rva_);
            wr32(out, dirs_ + 20, size);
            wi.placement = "in place";
            wi.rsrc_rva = rsrc_rva_;
            wi.rsrc_size = uint32_t(rs.size());
        } else {
            // Sections after the resources, by address.
            std::vector<std::size_t> after;
            for (std::size_t i = 0; i < secs.size(); ++i)
                if (secs[i].virtual_address > secs[si].virtual_address) after.push_back(i);
            bool starts_section = rsrc_rva_ == secs[si].virtual_address;
            // Only .reloc may move: nothing refers to it but its directory.
            bool movable = starts_section && allow_moving_reloc;
            for (std::size_t i : after) {
                if (secs[i].name != ".reloc") movable = false;
            }
            for (uint32_t d = 0; movable && d < num_dirs_; ++d) {
                if (d == 2 || d == 4 || d == 5) continue;
                uint32_t rva = rd32(bytes_, dirs_ + 8 * d);
                if (!rva) continue;
                for (std::size_t i : after)
                    if (rva >= secs[i].virtual_address && rva < secs[i].virtual_address + std::max(secs[i].virtual_size, secs[i].raw_size))
                        movable = false;
            }
            if (movable) {
                Section& s = secs[si];
                uint32_t new_raw = align(uint32_t(rs.size()), file_align_);
                uint32_t new_vend = align(s.virtual_address + uint32_t(rs.size()), sect_align_);
                // Gather the moved sections' raw bytes and the overlay.
                uint32_t old_raw_end_rsrc = s.raw_pointer + s.raw_size;
                uint32_t tail_end = raw_end();
                std::vector<uint8_t> tail(bytes_.begin() + std::min<std::size_t>(old_raw_end_rsrc, bytes_.size()),
                                          bytes_.begin() + std::min<std::size_t>(tail_end, bytes_.size()));
                std::vector<uint8_t> overlay;
                if (out.size() > tail_end) overlay.assign(out.begin() + tail_end, out.end());
                int64_t df = int64_t(s.raw_pointer + new_raw) - int64_t(old_raw_end_rsrc);
                uint32_t first_after_va = UINT32_MAX;
                for (std::size_t i : after) first_after_va = std::min(first_after_va, secs[i].virtual_address);
                int64_t dv = int64_t(std::max(new_vend, first_after_va)) - int64_t(first_after_va);
                out.resize(s.raw_pointer);
                out.insert(out.end(), rs.begin(), rs.end());
                out.resize(s.raw_pointer + new_raw, 0);
                out.insert(out.end(), tail.begin(), tail.end());
                out.insert(out.end(), overlay.begin(), overlay.end());
                s.raw_size = new_raw;
                s.virtual_size = uint32_t(rs.size());
                put_section(si);
                for (std::size_t i : after) {
                    if (secs[i].raw_size) secs[i].raw_pointer = uint32_t(int64_t(secs[i].raw_pointer) + df);
                    secs[i].virtual_address = uint32_t(int64_t(secs[i].virtual_address) + dv);
                    put_section(i);
                }
                uint32_t reloc = rd32(bytes_, dirs_ + 40);
                if (reloc) wr32(out, dirs_ + 40, uint32_t(int64_t(reloc) + dv));
                wr32(out, dirs_ + 16, s.virtual_address);
                wr32(out, dirs_ + 20, uint32_t(rs.size()));
                wi.placement = after.empty() ? "grown (last section)" : "grown, .reloc moved";
                wi.rsrc_rva = s.virtual_address;
                wi.rsrc_size = uint32_t(rs.size());
            } else {
                // A new section at the end.
                uint32_t room = std::min<uint32_t>(size_of_headers_, secs.empty() ? size_of_headers_ : UINT32_MAX);
                for (auto& x : secs)
                    if (x.raw_size) room = std::min(room, x.raw_pointer);
                uint32_t table_end = sect_table_ + 40u * uint32_t(secs.size());
                if (table_end + 40 > room)
                    throw Error("no room for another section header, and the resource section can't grow");
                for (int k = 0; k < 40; ++k)
                    if (out[table_end + uint32_t(k)] != 0)
                        throw Error("no free space for another section header");
                Section n;
                n.name = ".rsrc";
                n.virtual_address = image_end();
                rs = layout(n.virtual_address);
                n.virtual_size = uint32_t(rs.size());
                uint32_t rend = raw_end();
                std::vector<uint8_t> overlay;
                if (out.size() > rend) overlay.assign(out.begin() + rend, out.end());
                out.resize(rend);
                n.raw_pointer = align(rend, file_align_);
                n.raw_size = align(uint32_t(rs.size()), file_align_);
                n.characteristics = 0x40000040;  // initialized data, readable
                out.resize(n.raw_pointer, 0);
                out.insert(out.end(), rs.begin(), rs.end());
                out.resize(n.raw_pointer + n.raw_size, 0);
                out.insert(out.end(), overlay.begin(), overlay.end());
                secs.push_back(n);
                put_section(secs.size() - 1);
                wr16(out, pe_ + 6, uint16_t(secs.size()));
                wr32(out, dirs_ + 16, n.virtual_address);
                wr32(out, dirs_ + 20, n.virtual_size);
                wi.placement = "new section";
                wi.rsrc_rva = n.virtual_address;
                wi.rsrc_size = n.virtual_size;
                wi.warnings.push_back("the old resource section is left in place, unused");
            }
        }
    } else {
        throw Error("the image has no resource section to rewrite");
    }

    // SizeOfImage only ever grows: some linkers round it differently.
    wr32(out, opt_ + 56, std::max(rd32(bytes_, opt_ + 56), image_end()));
    if (had_checksum) {
        wr32(out, ck_off, rd32(bytes_, ck_off));
        if (out == bytes_) return out;   // unchanged: keep the stored sum
        wr32(out, ck_off, checksum(out, ck_off));
    }
    return out;
}

// ---------------------------------------------------------------------------
// Text

std::string to_utf8(const std::u16string& s) {
    std::string o;
    for (std::size_t i = 0; i < s.size(); ++i) {
        uint32_t c = s[i];
        if (c >= 0xD800 && c < 0xDC00 && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] < 0xE000) {
            c = 0x10000 + ((c - 0xD800) << 10) + (uint32_t(s[i + 1]) - 0xDC00);
            ++i;
        }
        if (c < 0x80) {
            o.push_back(char(c));
        } else if (c < 0x800) {
            o.push_back(char(0xC0 | (c >> 6)));
            o.push_back(char(0x80 | (c & 63)));
        } else if (c < 0x10000) {
            o.push_back(char(0xE0 | (c >> 12)));
            o.push_back(char(0x80 | ((c >> 6) & 63)));
            o.push_back(char(0x80 | (c & 63)));
        } else {
            o.push_back(char(0xF0 | (c >> 18)));
            o.push_back(char(0x80 | ((c >> 12) & 63)));
            o.push_back(char(0x80 | ((c >> 6) & 63)));
            o.push_back(char(0x80 | (c & 63)));
        }
    }
    return o;
}

std::u16string from_utf8(const std::string& s) {
    std::u16string o;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = uint8_t(s[i]);
        uint32_t cp;
        int n;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 31; n = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 15; n = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 7; n = 4; }
        else { cp = 0xFFFD; n = 1; }
        if (i + std::size_t(n) > s.size()) { cp = 0xFFFD; n = 1; }
        else
            for (int k = 1; k < n; ++k) cp = (cp << 6) | (uint8_t(s[i + std::size_t(k)]) & 63);
        i += std::size_t(n);
        if (cp >= 0x10000) {
            cp -= 0x10000;
            o.push_back(char16_t(0xD800 + (cp >> 10)));
            o.push_back(char16_t(0xDC00 + (cp & 1023)));
        } else {
            o.push_back(char16_t(cp));
        }
    }
    return o;
}

std::string language_name(uint16_t l) {
    switch (l) {
        case 0x0000: return "Neutral";
        case 0x0409: return "English (United States)";
        case 0x0809: return "English (United Kingdom)";
        case 0x0407: return "German";
        case 0x040c: return "French";
        case 0x040a: return "Spanish (Traditional)";
        case 0x0c0a: return "Spanish";
        case 0x0410: return "Italian";
        case 0x041d: return "Swedish";
        case 0x0411: return "Japanese";
        case 0x0412: return "Korean";
        case 0x0404: return "Chinese (Traditional)";
        case 0x0804: return "Chinese (Simplified)";
        case 0x0415: return "Polish";
        case 0x0419: return "Russian";
        case 0x0405: return "Czech";
        case 0x0413: return "Dutch";
        case 0x0416: return "Portuguese (Brazil)";
        default: break;
    }
    char b[16];
    std::snprintf(b, sizeof b, "0x%04x", l);
    return b;
}

}  // namespace lithtech::pe
