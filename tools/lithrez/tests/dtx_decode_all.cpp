// Decodes mip 0 of every .DTX in the given .rez files (or loose .dtx files
// and folders of them, such as an extracted PS2 disc) and prints, per
// texture, its format and an FNV-1a hash of the RGBA (for comparing with
// the Python decoder), then failures by kind. --ps2-alpha reads palette
// alpha as the PS2 does (lithtech::dtx::PaletteAlpha::Ps2).
//   dtx_decode_all [--hash] [--ps2-alpha] <file.rez | file.dtx | folder>...
#include <lithtech/dtx.hpp>
#include <lithtech/rez.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
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
    dtx::PaletteAlpha pa = dtx::PaletteAlpha::Stored;
    int ok = 0, bad = 0, total = 0;
    std::map<std::string, int> by_bpp, errors;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--hash")) { hash = true; continue; }
        if (!std::strcmp(argv[i], "--ps2-alpha")) { pa = dtx::PaletteAlpha::Ps2; continue; }
        namespace fs = std::filesystem;
        auto loose = [&](const fs::path& f) {
            ++total;
            try {
                auto t = dtx::read_file(f.string(), true);
                auto img = t.decode(0, 0, pa);
                ++ok;
                by_bpp[dtx::bpp_name(t.bpp())]++;
                if (hash)
                    std::printf("%s\t%s\t%016llx\n", f.string().c_str(), dtx::bpp_name(t.bpp()), (unsigned long long)fnv(img.rgba));
            } catch (const std::exception& e) {
                ++bad;
                errors[e.what()]++;
                std::fprintf(stderr, "FAIL %s: %s\n", f.string().c_str(), e.what());
            }
        };
        auto is_dtx = [](const fs::path& f) {
            std::string e = f.extension().string();
            for (auto& c : e) c = char(std::tolower(uint8_t(c)));
            return e == ".dtx";
        };
        if (fs::is_directory(argv[i])) {
            std::vector<fs::path> files;
            for (auto& e : fs::recursive_directory_iterator(argv[i]))
                if (e.is_regular_file() && is_dtx(e.path())) files.push_back(e.path());
            std::sort(files.begin(), files.end());
            for (auto& f : files) loose(f);
            continue;
        }
        if (is_dtx(argv[i])) { loose(argv[i]); continue; }
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
                    auto img = t.decode(0, 0, pa);
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
