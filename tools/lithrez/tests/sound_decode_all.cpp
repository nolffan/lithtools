// Decodes every .WAV in the given .rez files and prints counts by codec and
// the failures. --list prints a line per sound (codec, channels, rate,
// frames); --dump writes one sound as s16le, for comparing with FFmpeg
// (tests/compare_sound.py).
//   sound_decode_all [--list] <file.rez>...
//   sound_decode_all --dump <file.rez> <path in rez> <out.raw>
//   sound_decode_all --dump <file.wav> - <out.raw>
#include <lithtech/rez.hpp>
#include <lithtech/sound.hpp>

#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>

using namespace lithtech;

static std::vector<uint8_t> find_resource(const char* rezfile, const std::string& path) {
    auto a = rez::Archive::open(rezfile);
    std::function<const rez::Resource*(const rez::Directory&)> find = [&](const rez::Directory& d) -> const rez::Resource* {
        for (auto& r : d.resources())
            if (r->path('/') == path) return r.get();
        for (auto& s : d.dirs())
            if (auto* r = find(*s)) return r;
        return nullptr;
    };
    const rez::Resource* r = find(a->root());
    if (!r) { std::fprintf(stderr, "%s: no %s\n", rezfile, path.c_str()); return {}; }
    return r->read();
}

static int dump(const char* rezfile, const std::string& path, const char* out) {
    std::vector<uint8_t> bytes;
    if (path == "-") {
        std::FILE* in = std::fopen(rezfile, "rb");
        if (!in) { std::perror(rezfile); return 2; }
        uint8_t buf[65536];
        for (std::size_t n; (n = std::fread(buf, 1, sizeof buf, in)) > 0;) bytes.insert(bytes.end(), buf, buf + n);
        std::fclose(in);
    } else {
        bytes = find_resource(rezfile, path);
        if (bytes.empty()) return 2;
    }
    sound::Info info;
    auto pcm = sound::decode(bytes.data(), bytes.size(), &info);
    std::FILE* f = std::fopen(out, "wb");
    if (!f) return 2;
    std::fwrite(pcm.samples.data(), 2, pcm.samples.size(), f);   // little-endian hosts only
    std::fclose(f);
    std::printf("%s %d %d %zu\n", sound::codec_name(info.format_tag), pcm.channels, pcm.sample_rate, pcm.frames());
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 5 && !std::strcmp(argv[1], "--dump")) {
        try { return dump(argv[2], argv[3], argv[4]); } catch (const std::exception& e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 1;
        }
    }
    bool list = false;
    int ok = 0, bad = 0, total = 0, empty = 0;
    double seconds = 0, longest = 0;
    std::string longest_name;
    std::map<std::string, int> by_codec, errors;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--list")) { list = true; continue; }
        std::unique_ptr<rez::Archive> a;
        try { a = rez::Archive::open(argv[i]); } catch (const std::exception& e) {
            std::fprintf(stderr, "%s: %s\n", argv[i], e.what());
            continue;
        }
        std::function<void(const rez::Directory&)> walk = [&](const rez::Directory& d) {
            for (auto& r : d.resources()) {
                if (r->type_string() != "WAV") continue;
                ++total;
                try {
                    auto bytes = r->read();
                    sound::Info info;
                    auto pcm = sound::decode(bytes.data(), bytes.size(), &info);
                    if (!pcm.frames() && !info.data_bytes) { ++empty; continue; }   // a header with no data
                    if (!pcm.frames()) throw sound::Error("no samples");
                    ++ok;
                    by_codec[sound::codec_name(info.format_tag)]++;
                    seconds += pcm.seconds();
                    if (pcm.seconds() > longest) { longest = pcm.seconds(); longest_name = std::string(argv[i]) + " " + r->path('/'); }
                    if (list)
                        std::printf("%s\t%s\t%s\t%d\t%d\t%zu\n", argv[i], r->path('/').c_str(), sound::codec_name(info.format_tag),
                                    pcm.channels, pcm.sample_rate, pcm.frames());
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
    std::fprintf(stderr, "%d sounds: %d decoded, %d empty, %d failed; %.1f hours in all, longest %.1f s (%s)\n", total, ok,
                 empty, bad,
                 seconds / 3600, longest, longest_name.c_str());
    for (auto& [k, n] : by_codec) std::fprintf(stderr, "  %-16s %d\n", k.c_str(), n);
    for (auto& [k, n] : errors) std::fprintf(stderr, "  error %d x %s\n", n, k.c_str());
    return bad ? 1 : 0;
}
