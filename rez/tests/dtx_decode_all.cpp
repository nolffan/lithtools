// Decodes mip 0 of every .DTX in the given .rez files and prints, per
// texture, its format and an FNV-1a hash of the RGBA (for comparing with
// the Python decoder), then failures by kind.
//   dtx_decode_all [--hash] <file.rez>...
#include <lithtech/dtx.hpp>
#include <lithtech/rez.hpp>

#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>

using namespace lithtech;

static uint64_t fnv(const std::vector<uint8_t>& v) {
    uint64_t h = 1469598103934665603ull;
    for (uint8_t b : v) { h ^= b; h *= 1099511628211ull; }
    return h;
}

int main(int argc, char** argv) {
    bool hash = false;
    int ok = 0, bad = 0, total = 0;
    std::map<std::string, int> by_bpp, errors;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--hash")) { hash = true; continue; }
        std::unique_ptr<rez::Archive> a;
        try { a = rez::Archive::open(argv[i]); } catch (const std::exception& e) {
            std::fprintf(stderr, "%s: %s\n", argv[i], e.what());
            continue;
        }
        std::function<void(const rez::Directory&)> walk = [&](const rez::Directory& d) {
            for (auto& r : d.resources()) {
                if (r->type_string() != "DTX") continue;
                ++total;
                try {
                    auto t = dtx::read(r->read(), true);
                    auto img = t.decode(0);
                    if (t.is_cubemap() && t.find_section("CUBEMAPDATA"))
                        for (int f = 1; f < 6; ++f) t.decode(0, f);
                    ++ok;
                    by_bpp[dtx::bpp_name(t.bpp())]++;
                    if (hash)
                        std::printf("%s\t%s\t%s\t%016llx\n", argv[i], r->path('/').c_str(), dtx::bpp_name(t.bpp()),
                                    (unsigned long long)fnv(img.rgba));
                } catch (const std::exception& e) {
                    ++bad;
                    errors[e.what()]++;
                    std::fprintf(stderr, "FAIL %s %s: %s\n", argv[i], r->path('/').c_str(), e.what());
                }
            }
            for (auto& s : d.dirs()) walk(*s);
        };
        walk(a->root());
    }
    std::fprintf(stderr, "%d textures: %d decoded, %d failed\n", total, ok, bad);
    for (auto& [k, n] : by_bpp) std::fprintf(stderr, "  %-5s %d\n", k.c_str(), n);
    for (auto& [k, n] : errors) std::fprintf(stderr, "  error %d x %s\n", n, k.c_str());
    return bad ? 1 : 0;
}
