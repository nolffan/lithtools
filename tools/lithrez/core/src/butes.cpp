// PS2 compiled attribute files. Layout (little-endian):
//   u32 tag count
//   per tag: u32 name hash, u32 offset of its keys, u32 key count
//   keys: u16 record size (8 + value, a multiple of 4), u16 type,
//         u32 name hash, value
// Values: string (NUL-terminated, padded), int (s32), float, vector (3
// floats), point (2 x s32), bool (u32), range (2 floats), rect (4 x s32).

#include "lithtech/butes.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_map>

namespace lithtech::butes {

// The compiled-in names (generated from core/data/ps2-bute-names.txt).
extern const char kButeNamesText[];
extern const std::size_t kButeNamesTextSize;

namespace {

uint32_t rd32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
float rdf(const uint8_t* p) {
    uint32_t u = rd32(p);
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

std::string num(float f) {
    char b[32];
    std::snprintf(b, sizeof b, "%.6f", double(f));
    std::string s = b;
    // 0.750000 -> 0.75, 1.000000 -> 1.0
    while (s.size() > 1 && s.back() == '0' && s[s.size() - 2] != '.') s.pop_back();
    return s;
}

const std::unordered_map<uint32_t, std::string>& names() {
    static const auto m = [] {
        std::unordered_map<uint32_t, std::string> x;
        std::string_view t(kButeNamesText, kButeNamesTextSize);
        std::size_t p = 0;
        while (p < t.size()) {
            std::size_t e = t.find('\n', p);
            if (e == std::string_view::npos) e = t.size();
            std::string line(t.substr(p, e - p));
            p = e + 1;
            while (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            x.emplace(hash(line), line);
        }
        return x;
    }();
    return m;
}

}  // namespace

uint32_t hash(std::string_view name) {
    uint32_t h = 0;
    for (char c : name) h = h * 31 + uint8_t(c);
    return h;
}

std::vector<Tag> read(const uint8_t* d, std::size_t n) {
    if (n < 4) throw Error("too short for an attribute file");
    uint32_t count = rd32(d);
    if (count == 0 || count > 100000 || 4 + std::size_t(count) * 12 > n) throw Error("bad tag count");
    std::vector<Tag> tags(count);
    std::size_t end_of_keys = 4 + std::size_t(count) * 12;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* t = d + 4 + 12 * i;
        tags[i].hash = rd32(t);
        std::size_t p = rd32(t + 4);
        uint32_t keys = rd32(t + 8);
        if (keys > 100000) throw Error("bad key count");
        for (uint32_t k = 0; k < keys; ++k) {
            if (p + 8 > n) throw Error("a key runs past the end");
            uint16_t size = rd16(d + p), type = rd16(d + p + 2);
            if (size < 8 || p + size > n || type < String || type > Rect) throw Error("bad key record");
            static const uint8_t fixed[] = {0, 0, 4, 4, 12, 8, 4, 8, 16};
            if (type != String && size != 8 + fixed[type]) throw Error("bad value size");
            if (type == String && !std::memchr(d + p + 8, 0, size - 8)) throw Error("unterminated string");
            Key key;
            key.hash = rd32(d + p + 4);
            key.type = type;
            key.value.assign(d + p + 8, d + p + size);
            tags[i].keys.push_back(std::move(key));
            p += size;
        }
        end_of_keys = std::max(end_of_keys, p);
    }
    if (end_of_keys != n) throw Error("bytes after the last key");
    return tags;
}

bool looks_like(const uint8_t* data, std::size_t size) {
    try {
        read(data, size);
        return true;
    } catch (const Error&) {
        return false;
    }
}

std::string name_of(uint32_t h) {
    auto& m = names();
    auto it = m.find(h);
    if (it != m.end()) return it->second;
    char b[16];
    std::snprintf(b, sizeof b, "#%08X", h);
    return b;
}

std::string to_text(const std::vector<Tag>& tags) {
    std::string out;
    for (auto& t : tags) {
        if (!out.empty()) out += "\n";
        out += "[" + name_of(t.hash) + "]\n";
        for (auto& k : t.keys) {
            const uint8_t* v = k.value.data();
            std::string val;
            switch (k.type) {
                case String: val = "\"" + std::string(reinterpret_cast<const char*>(v)) + "\""; break;
                case Int: val = std::to_string(int32_t(rd32(v))); break;
                case Float: val = num(rdf(v)); break;
                case Vector: val = "<" + num(rdf(v)) + ", " + num(rdf(v + 4)) + ", " + num(rdf(v + 8)) + ">"; break;
                case Point: val = "(" + std::to_string(int32_t(rd32(v))) + ", " + std::to_string(int32_t(rd32(v + 4))) + ")"; break;
                case Bool: val = rd32(v) ? "TRUE" : "FALSE"; break;
                case Range: val = "[" + num(rdf(v)) + ", " + num(rdf(v + 4)) + "]"; break;
                case Rect:
                    val = "(" + std::to_string(int32_t(rd32(v))) + ", " + std::to_string(int32_t(rd32(v + 4))) + ", " +
                          std::to_string(int32_t(rd32(v + 8))) + ", " + std::to_string(int32_t(rd32(v + 12))) + ")";
                    break;
            }
            out += name_of(k.hash) + " = " + val + "\n";
        }
    }
    return out;
}

}  // namespace lithtech::butes
