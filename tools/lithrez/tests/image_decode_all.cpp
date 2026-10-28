// Decodes every .PCX, .TGA and .BMP in the given .rez files (or loose files
// and folders of them) and prints, per image, its layout and size, then the
// totals and failures. --dump <dir> also writes each image's RGBA (top row
// first) to <dir>/<n>.rgba, with <dir>/index.tsv naming them, for
// compare_images.py to check against ImageMagick.
//   image_decode_all [--dump <dir>] <file.rez | image | folder>...
#include <lithtech/bmp.hpp>
#include <lithtech/pcx.hpp>
#include <lithtech/rez.hpp>
#include <lithtech/tga.hpp>

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
    // The decoded image, whichever format: size, RGBA, a layout name.
    struct Decoded {
        int width = 0, height = 0;
        std::vector<uint8_t> rgba;
        std::string layout;
        bool truncated = false;
    };
    auto decode = [](const std::string& ext, const std::vector<uint8_t>& data) {
        Decoded o;
        if (ext == "pcx") {
            auto i = pcx::read(data);
            o = {i.width, i.height, std::move(i.rgba),
                 "PCX " + pcx::layout_name(i.header) + (i.palette.empty() ? "" : ", " + i.palette + " palette"),
                 i.truncated};
        } else if (ext == "tga") {
            auto i = tga::read(data);
            o = {i.width, i.height, std::move(i.rgba), "TGA " + tga::layout_name(i.header), i.truncated};
        } else {
            auto i = bmp::read(data);
            o = {i.width, i.height, std::move(i.rgba),
                 "BMP " + bmp::layout_name(i.header) + ", " + bmp::header_name(i.header.header_size), i.truncated};
        }
        return o;
    };
    auto one = [&](const std::string& name, const std::string& ext, const std::vector<uint8_t>& data,
                   const std::string& loose) {
        try {
            auto img = decode(ext, data);
            ++ok;
            ++layouts[img.layout];
            std::printf("%-60s %-40s %dx%d%s\n", name.c_str(), img.layout.c_str(), img.width, img.height,
                        img.truncated ? " TRUNCATED" : "");
            if (!dump.empty()) {
                std::string f = dump + "/" + std::to_string(n) + ".rgba";
                std::ofstream(f, std::ios::binary).write(reinterpret_cast<const char*>(img.rgba.data()),
                                                         std::streamsize(img.rgba.size()));
                std::string src = loose;
                if (src.empty()) {
                    src = dump + "/" + std::to_string(n) + "." + ext;
                    std::ofstream(src, std::ios::binary).write(reinterpret_cast<const char*>(data.data()),
                                                               std::streamsize(data.size()));
                }
                index << n << '\t' << img.width << '\t' << img.height << '\t' << ext << '\t' << src << '\t' << name
                      << '\n';
                ++n;
            }
        } catch (const std::exception& e) {
            ++bad;
            ++errors[ext + ": " + e.what()];
            std::printf("%-60s FAILED: %s\n", name.c_str(), e.what());
        }
    };
    auto ext_of = [](std::string e) {   // "pcx", "tga", "bmp", or ""
        if (!e.empty() && e[0] == '.') e.erase(0, 1);
        std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return e == "pcx" || e == "tga" || e == "bmp" ? e : std::string();
    };
    auto loose = [&](const fs::path& f) {
        std::ifstream in(f, std::ios::binary);
        std::vector<uint8_t> v((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        one(f.string(), ext_of(f.extension().string()), v, fs::absolute(f).string());
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
                if (e.is_regular_file() && !ext_of(e.path().extension().string()).empty()) loose(e.path());
        } else if (!ext_of(p.extension().string()).empty()) {
            loose(p);
        } else {
            auto a = rez::Archive::open(p.string());
            std::function<void(const rez::Directory&)> walk = [&](const rez::Directory& d) {
                for (auto& r : d.resources())
                    if (std::string e = ext_of(r->type_string()); !e.empty())
                        one(p.filename().string() + ":" + r->path('/'), e, r->read(), "");
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
