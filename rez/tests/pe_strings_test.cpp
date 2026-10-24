// Checks lithtech::pe on real resource DLLs:
//   pe_strings_test <dll> [edited-out.dll] [ids-out.txt]
// parses, round-trips unedited (bytes must match), then edits (longer
// strings, an insert, a delete), writes, re-parses and compares. With
// output paths, writes the edited DLL and "id<TAB>utf8" lines of the ids it
// changed, for checking under Windows.
#include <lithtech/pe_strings.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <functional>
#include <iterator>

using namespace lithtech;

static std::vector<uint8_t> load(const char* p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: pe_strings_test <dll> [out.dll] [ids.txt]\n"); return 2; }
    int fails = 0;
    try {
        auto bytes = load(argv[1]);
        auto img = pe::Image::parse(bytes);
        auto langs = img.string_languages();
        std::printf("%s: %s, %zu sections, %zu resource types, languages:", argv[1], img.pe32_plus() ? "PE32+" : "PE32",
                    img.sections().size(), img.resources().children.size());
        for (auto l : langs) std::printf(" %s(%zu strings)", pe::language_name(l).c_str(), img.strings(l).size());
        std::printf("\n");
        if (!img.has_resources()) { std::printf("  no resources\n"); return 0; }

        pe::WriteInfo wi;
        auto same = img.write(&wi);
        bool ident = same == bytes;
        std::printf("  layout %s\n", img.layout_matches_original() ? "byte-identical to the stored section" : "differs from the stored section");
        if (const char* dump = std::getenv("PE_DUMP_UNEDITED"))
            std::ofstream(dump, std::ios::binary).write(reinterpret_cast<const char*>(same.data()), std::streamsize(same.size()));
        std::printf("  unedited write: %s (%s)\n", ident ? "byte-identical" : "DIFFERS", wi.placement.c_str());
        if (!ident) {
            fails++;
            std::size_t i = 0;
            while (i < same.size() && i < bytes.size() && same[i] == bytes[i]) ++i;
            std::printf("  first difference at %zu (sizes %zu/%zu)\n", i, bytes.size(), same.size());
        }
        if (langs.empty() && std::getenv("PE_ADD")) {
            // Give it a table to edit (the PE32+ path has no game DLL).
            pe::StringTable add;
            for (uint32_t k = 100; k < 140; ++k) add[k] = u"added " + pe::from_utf8(std::to_string(k));
            img.set_strings(0x0409, add);
            langs = img.string_languages();
            bytes = img.write();
            img = pe::Image::parse(bytes);
        }
        if (langs.empty()) return fails ? 1 : 0;

        uint16_t lang = langs.front();
        if (const char* want = std::getenv("PE_LANG")) lang = uint16_t(std::strtoul(want, nullptr, 0));
        auto t = img.strings(lang);
        auto edited = t;
        std::vector<uint32_t> changed;
        int n = 0;
        for (auto& [id, s] : edited) {
            if (n++ % 50 == 0) {
                s += u" [edited by lithtools: a much longer string than before, to make the section grow]";
                changed.push_back(id);
            }
        }
        // Insert after the highest id and delete one.
        uint32_t top = edited.rbegin()->first + 1;
        edited[top] = u"New string éü 中";
        changed.push_back(top);
        uint32_t del = edited.begin()->first;
        edited.erase(del);
        // Lots of growth: 200 KB of new strings beyond the slack.
        for (uint32_t k = 0; k < 64; ++k) edited[60000 + k] = std::u16string(1600, u'A' + char16_t(k % 26));
        changed.push_back(60000);
        img.set_strings(lang, edited);
        pe::WriteInfo wi2;
        auto out = img.write(&wi2);
        auto back = pe::Image::parse(out);
        bool eq = back.strings(lang) == edited;
        std::printf("  edited write: %zu -> %zu bytes, %s, re-parse %s", bytes.size(), out.size(), wi2.placement.c_str(),
                    eq ? "matches" : "MISMATCH");
        for (auto& w : wi2.warnings) std::printf("; %s", w.c_str());
        std::printf("\n");
        if (!eq) fails++;
        // Other resources untouched.
        auto strip = [](pe::Node n) {
            n.children.erase(std::remove_if(n.children.begin(), n.children.end(),
                                            [](const pe::Node& c) { return c.name.is_id && c.name.id == pe::RT_STRING; }),
                             n.children.end());
            return n;
        };
        std::function<bool(const pe::Node&, const pe::Node&)> same_tree = [&](const pe::Node& a, const pe::Node& b) {
            if (!(a.name == b.name) || a.is_dir != b.is_dir || a.data != b.data || a.codepage != b.codepage ||
                a.children.size() != b.children.size())
                return false;
            for (std::size_t i = 0; i < a.children.size(); ++i)
                if (!same_tree(a.children[i], b.children[i])) return false;
            return true;
        };
        auto orig = pe::Image::parse(bytes);
        bool others = same_tree(strip(orig.resources()), strip(back.resources()));
        std::printf("  other resources %s\n", others ? "unchanged" : "CHANGED");
        if (!others) fails++;
        // Unedited write of the edited image is stable.
        if (!back.layout_matches_original()) { std::printf("  edited image's layout not stable\n"); fails++; }

        {
            pe::WriteInfo wi3;
            auto ns = img.write(&wi3, false);
            auto b3 = pe::Image::parse(ns);
            bool ok3 = b3.strings(lang) == edited && b3.write() == ns;
            std::printf("  new-section write: %zu bytes, %s, %s\n", ns.size(), wi3.placement.c_str(), ok3 ? "re-parse matches" : "MISMATCH");
            if (!ok3) fails++;
            if (argc > 4)
                std::ofstream(argv[4], std::ios::binary).write(reinterpret_cast<const char*>(ns.data()), std::streamsize(ns.size()));
        }
        if (argc > 2) {
            std::ofstream(argv[2], std::ios::binary).write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
        }
        if (argc > 3) {
            std::ofstream f(argv[3]);
            auto esc = [](std::string x) {
                std::string o;
                for (char c : x) o += c == '\n' ? std::string("\\n") : c == '\r' ? std::string("\\r") : std::string(1, c);
                return o;
            };
            for (auto id : changed) f << id << '\t' << esc(pe::to_utf8(edited[id])) << '\n';
            f << del << '\t' << "(deleted)\n";
        }
    } catch (const std::exception& e) {
        std::printf("  ERROR %s\n", e.what());
        return 1;
    }
    return fails ? 1 : 0;
}
