// Decodes every .WAV and every sound of every PS2 bank (.LPS) in the given
// .rez files, folders (searched recursively) or loose files, and prints
// counts by codec and the failures. A bank sound's loop must agree with its
// record. --list prints a line per sound (codec, channels, rate, frames);
// --dump writes one sound as s16le, for comparing with FFmpeg
// (tests/compare_sound.py) or the PS2 branch's decoder (tests/compare_lps.py).
//   sound_decode_all [--list] <file.rez | folder | file.wav | file.lps>...
//   sound_decode_all --dump <file.rez> <path in rez> <out.raw>
//   sound_decode_all --dump <file.wav> - <out.raw>
//   sound_decode_all --dump-bank <file.lps> <out dir>   every sound as <index>.raw
#include <lithtech/rez.hpp>
#include <lithtech/sound.hpp>

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

static std::vector<uint8_t> read_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

static int dump_bank(const char* file, const char* outdir) {
    auto bytes = read_file(file);
    auto bank = sound::read_bank(bytes.data(), bytes.size());
    std::filesystem::create_directories(outdir);
    for (std::size_t i = 0; i < bank.sounds.size(); ++i) {
        auto pcm = sound::decode_bank_sound(bank, i);
        std::FILE* f = std::fopen((std::filesystem::path(outdir) / (std::to_string(i) + ".raw")).string().c_str(), "wb");
        if (!f) return 2;
        std::fwrite(pcm.samples.data(), 2, pcm.samples.size(), f);   // little-endian hosts only
        std::fclose(f);
    }
    std::printf("%zu sounds\n", bank.sounds.size());
    return 0;
}

static std::string upper_ext(const std::string& name) {
    auto dot = name.rfind('.');
    std::string e = dot == std::string::npos ? "" : name.substr(dot + 1);
    for (auto& c : e) c = char(std::toupper(static_cast<unsigned char>(c)));
    return e;
}

int main(int argc, char** argv) {
    if (argc == 5 && !std::strcmp(argv[1], "--dump")) {
        try { return dump(argv[2], argv[3], argv[4]); } catch (const std::exception& e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 1;
        }
    }
    if (argc == 4 && !std::strcmp(argv[1], "--dump-bank")) {
        try { return dump_bank(argv[2], argv[3]); } catch (const std::exception& e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 1;
        }
    }
    bool list = false;
    int ok = 0, bad = 0, total = 0, empty = 0, banks = 0, loops = 0;
    double seconds = 0, longest = 0;
    std::string longest_name;
    std::map<std::string, int> by_codec, errors;
    auto fail = [&](const std::string& where, const char* what) {
        ++bad;
        errors[what]++;
        std::fprintf(stderr, "FAIL %s: %s\n", where.c_str(), what);
    };
    auto counted = [&](const std::string& where, const char* codec, const sound::Pcm& pcm) {
        ++ok;
        by_codec[codec]++;
        seconds += pcm.seconds();
        if (pcm.seconds() > longest) { longest = pcm.seconds(); longest_name = where; }
        if (list) std::printf("%s\t%s\t%d\t%d\t%zu\n", where.c_str(), codec, pcm.channels, pcm.sample_rate, pcm.frames());
    };
    // One file's bytes: a .wav, or a bank and each of its sounds.
    auto check = [&](const std::string& where, const std::string& ext, const std::vector<uint8_t>& bytes) {
        if (ext == "WAV") {
            ++total;
            try {
                sound::Info info;
                auto pcm = sound::decode(bytes.data(), bytes.size(), &info);
                if (!pcm.frames() && !info.data_bytes) { ++empty; return; }   // a header with no data
                if (!pcm.frames()) throw sound::Error("no samples");
                counted(where, sound::codec_name(info.format_tag), pcm);
            } catch (const std::exception& e) { fail(where, e.what()); }
        } else if (ext == "LPS") {
            sound::Bank bank;
            try { bank = sound::read_bank(bytes.data(), bytes.size()); } catch (const std::exception& e) {
                ++total;
                return fail(where, e.what());
            }
            ++banks;
            for (std::size_t i = 0; i < bank.sounds.size(); ++i) {
                ++total;
                const auto& s = bank.sounds[i];
                std::string at = where + " #" + std::to_string(i) + " " + s.path;
                try {
                    uint32_t ls = 0, le = 0;
                    auto pcm = sound::decode_bank_sound(bank, i, &ls, &le);
                    if (!pcm.frames()) throw sound::Error("no samples");
                    // the frames' loop flags against the record
                    if (s.looping() != (le > ls)) throw sound::Error("loop flags and record disagree");
                    if (s.looping() && ls != s.loop_start / 16 * 28) throw sound::Error("loop start and record disagree");
                    loops += s.looping();
                    counted(at, "PS-ADPCM", pcm);
                } catch (const std::exception& e) { fail(at, e.what()); }
            }
        }
    };
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--list")) { list = true; continue; }
        std::filesystem::path p(argv[i]);
        std::string ext = upper_ext(p.filename().string());
        if (std::filesystem::is_directory(p)) {
            std::vector<std::filesystem::path> files;
            for (auto& e : std::filesystem::recursive_directory_iterator(p))
                if (e.is_regular_file()) files.push_back(e.path());
            std::sort(files.begin(), files.end());
            for (auto& f : files) {
                std::string fe = upper_ext(f.filename().string());
                if (fe == "WAV" || fe == "LPS") check(f.string(), fe, read_file(f));
            }
            continue;
        }
        if (ext == "WAV" || ext == "LPS") {
            check(p.string(), ext, read_file(p));
            continue;
        }
        std::unique_ptr<rez::Archive> a;
        try { a = rez::Archive::open(argv[i]); } catch (const std::exception& e) {
            std::fprintf(stderr, "%s: %s\n", argv[i], e.what());
            continue;
        }
        std::function<void(const rez::Directory&)> walk = [&](const rez::Directory& d) {
            for (auto& r : d.resources()) {
                std::string t = r->type_string();
                if (t == "WAV" || t == "LPS") check(std::string(argv[i]) + " " + r->path('/'), t, r->read());
            }
            for (auto& s : d.dirs()) walk(*s);
        };
        walk(a->root());
    }
    std::fprintf(stderr, "%d sounds: %d decoded, %d empty, %d failed; %.1f hours in all, longest %.1f s (%s)\n", total, ok,
                 empty, bad, seconds / 3600, longest, longest_name.c_str());
    if (banks) std::fprintf(stderr, "  %d PS2 banks, %d looping sounds\n", banks, loops);
    for (auto& [k, n] : by_codec) std::fprintf(stderr, "  %-16s %d\n", k.c_str(), n);
    for (auto& [k, n] : errors) std::fprintf(stderr, "  error %d x %s\n", n, k.c_str());
    return bad ? 1 : 0;
}
