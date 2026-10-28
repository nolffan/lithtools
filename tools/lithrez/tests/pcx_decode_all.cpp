// Decodes every .PCX in the given .rez files (or loose .pcx files and
// folders of them) and prints, per image, its layout and size, then the
// totals and failures. --dump <dir> also writes each image's RGBA (top row
// first) to <dir>/<n>.rgba, with <dir>/index.tsv naming them, for
// compare_pcx.py to check against ImageMagick.
//   pcx_decode_all [--dump <dir>] <file.rez | file.pcx | folder>...
#include <lithtech/pcx.hpp>
#include <lithtech/rez.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <string>

using namespace lithtech;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::string dump;
    std::ofstream index;
    int ok = 0, bad = 0, n = 0;
    std::map<std::string, int> layouts, errors;
    auto one = [&](const std::string& name, const std::vector<uint8_t>& data, const std::string& loose) {
        try {
            auto img = pcx::read(data);
            ++ok;
            ++layouts[pcx::layout_name(img.header) + (img.palette.empty() ? "" : ", " + img.palette + " palette")];
            std::printf("%-60s %-28s %dx%d%s\n", name.c_str(), pcx::layout_name(img.header).c_str(), img.width,
                        img.height, img.truncated ? " TRUNCATED" : "");
            if (!dump.empty()) {
                std::string f = dump + "/" + std::to_string(n) + ".rgba";
                std::ofstream(f, std::ios::binary).write(reinterpret_cast<const char*>(img.rgba.data()),
                                                         std::streamsize(img.rgba.size()));
                if (loose.empty()) {
                    std::string src = dump + "/" + std::to_string(n) + ".pcx";
                    std::ofstream(src, std::ios::binary).write(reinterpret_cast<const char*>(data.data()),
                                                               std::streamsize(data.size()));
                    index << n << '\t' << img.width << '\t' << img.height << '\t' << src << '\t' << name << '\n';
                } else {
                    index << n << '\t' << img.width << '\t' << img.height << '\t' << loose << '\t' << name << '\n';
                }
                ++n;
            }
        } catch (const std::exception& e) {
            ++bad;
            ++errors[e.what()];
            std::printf("%-60s FAILED: %s\n", name.c_str(), e.what());
        }
    };
    auto is_pcx = [](const fs::path& p) {
        std::string e = p.extension().string();
        std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return e == ".pcx";
    };
    auto loose = [&](const fs::path& f) {
        std::ifstream in(f, std::ios::binary);
        std::vector<uint8_t> v((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        one(f.string(), v, fs::absolute(f).string());
    };
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--dump") && i + 1 < argc) {
            dump = argv[++i];
            fs::create_directories(dump);
            index.open(dump + "/index.tsv");
            continue;
        }
        fs::path p = argv[i];
        if (fs::is_directory(p)) {
            for (auto& e : fs::recursive_directory_iterator(p))
                if (e.is_regular_file() && is_pcx(e.path())) loose(e.path());
        } else if (is_pcx(p)) {
            loose(p);
        } else {
            auto a = rez::Archive::open(p.string());
            std::function<void(const rez::Directory&)> walk = [&](const rez::Directory& d) {
                for (auto& r : d.resources())
                    if (r->type_string() == "PCX") one(p.filename().string() + ":" + r->path('/'), r->read(), "");
                for (auto& s : d.dirs()) walk(*s);
            };
            walk(a->root());
        }
    }
    std::printf("\n%d decoded, %d failed\n", ok, bad);
    for (auto& [k, v] : layouts) std::printf("  %5d  %s\n", v, k.c_str());
    for (auto& [k, v] : errors) std::printf("  %5d  FAILED: %s\n", v, k.c_str());
    return bad ? 1 : 0;
}
