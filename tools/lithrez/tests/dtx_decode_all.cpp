// Decodes mip 0 of every .DTX in the given .rez files (or loose .dtx files
// and folders of them, such as an extracted PS2 disc) and prints, per
// texture, its format and an FNV-1a hash of the RGBA (for comparing with
// the Python decoder), then failures by kind. --ps2-alpha reads palette
// alpha as the PS2 does (lithtech::dtx::PaletteAlpha::Ps2). Versions -2 and
// -3 (Blood 2, KISS Psycho Circus) are counted by version and alpha masks,
// read strictly, every level decoded, and the file must end exactly where
// the layout says.
//   dtx_decode_all [--hash] [--ps2-alpha] <file.rez | file.dtx | folder>...
#include <lithtech/dtx.hpp>
#include <lithtech/rez.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <string>

using namespace lithtech;

// The format a texture is counted under: the BPP name for -5 (as before),
// "8P v-2" / "8P+mask v-3" and so on for the old versions.
static std::string kind(const dtx::Texture& t) {
    std::string k = dtx::bpp_name(t.bpp());
    if (!t.is_old()) return k;
    if (t.has_alpha_masks()) k += "+mask";
    return k + " v" + std::to_string(t.version);
}

// -2/-3: read strictly, decode every level and check the size adds up.
static void check_old(const std::vector<uint8_t>& d, const dtx::Texture& lenient) {
    if (!lenient.is_old()) return;
    auto t = dtx::read(d, false);
    std::size_t n = (t.version == -3 ? 164 : 36) + 8 + 1024;
    for (int i = 0; i < t.mip_count; ++i) {
        n += std::size_t(t.width >> i) * std::size_t(t.height >> i);
        if (t.has_alpha_masks()) n += dtx::old_mask_size(t.width >> i, t.height >> i);
        t.decode(i);
    }
    for (auto& s : t.sections) n += 32 + s.data.size();
    if (n != d.size()) throw dtx::Error("layout is " + std::to_string(n) + " bytes, the file " + std::to_string(d.size()));
}

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
                if (t.is_old()) {
                    std::ifstream in(f, std::ios::binary);
                    check_old(std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()), t);
                }
                ++ok;
                by_bpp[kind(t)]++;
                if (hash)
                    std::printf("%s\t%s\t%016llx\n", f.string().c_str(), kind(t).c_str(), (unsigned long long)fnv(img.rgba));
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
                    auto data = r->read();
                    auto t = dtx::read(data, true);
                    auto img = t.decode(0, 0, pa);
                    check_old(data, t);
                    if (t.is_cubemap() && t.find_section("CUBEMAPDATA"))
                        for (int f = 1; f < 6; ++f) t.decode(0, f);
                    ++ok;
                    by_bpp[kind(t)]++;
                    if (hash)
                        std::printf("%s\t%s\t%s\t%016llx\n", argv[i], r->path('/').c_str(), kind(t).c_str(),
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
