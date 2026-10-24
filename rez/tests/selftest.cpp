// Builds a small tree, creates a rez from it, reads it back, extracts it and
// compares; checks the type packing, RezMgr's order and reproducible output.
#include <lithtech/rez.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;
using namespace lithtech;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)

static void put(const fs::path& p, const std::string& s) {
    fs::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary) << s;
}
static std::string get(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    fs::path w = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "rez-selftest";
    fs::remove_all(w);
    fs::path src = w / "src";
    put(src / "chars" / "skins" / "hero.dtx", "skin");
    put(src / "chars" / "models" / "hero.abc", "model data");
    put(src / "worlds" / "dm_test.dat", std::string(1000, 'x'));
    put(src / "123.txt", "id from name");
    put(src / "empty.wav", "");
    put(src / "readme.longext", "too long");
    put(src / "noext", "no extension");
    fs::create_directories(src / "emptydir");

    CHECK(rez::type_from_string("DTX") == 0x00445458u);
    CHECK(rez::type_to_string(0x00445458u) == "DTX");
    CHECK(rez::type_to_string(rez::type_from_string("LTB")) == "LTB");

    rez::CreateOptions co;
    co.time = 1000000000;
    rez::Report rep;
    auto a = rez::create_from_directory(src.string(), co, rep);
    CHECK(rep.errors == 0);
    CHECK(rep.warnings == 1);   // the long extension
    CHECK(a->root().count_resources() == 7);
    fs::path rz = w / "out.rez";
    rez::SaveOptions so;
    so.order = rez::EntryOrder::RezMgr;
    a->save(rz.string(), so);

    auto b = rez::Archive::open(rz.string());
    CHECK(b->header.user_title == "LithTech Resource File");
    CHECK(b->header.time == 1000000000);
    auto* r = b->root().find_path("chars/skins/hero.dtx");
    CHECK(r && r->name == "HERO" && r->type_string() == "DTX" && r->id == 10000000);
    if (r) { auto v = r->read(); CHECK(std::string(v.begin(), v.end()) == "skin"); }
    auto* n = b->root().find_path("123.txt");
    CHECK(n && n->id == 123);
    auto* l = b->root().find_path("README.LONGEXT");
    CHECK(l && l->type == 0);
    CHECK(b->root().find_dir_path("EMPTYDIR") != nullptr);

    // The same input and time give the same bytes.
    rez::Report rep2;
    auto c = rez::create_from_directory(src.string(), co, rep2);
    c->save((w / "again.rez").string(), so);
    CHECK(get(rz) == get(w / "again.rez"));

    // Unchanged read-and-write gives the same bytes.
    rez::SaveOptions keep;
    keep.recompute_largest = false;
    b->save((w / "rt.rez").string(), keep);
    CHECK(get(rz) == get(w / "rt.rez"));

    // Extract and compare.
    rez::Report rep3;
    auto d = rez::Archive::open(rz.string());
    rez::extract(d->root(), (w / "x").string(), rep3, false);
    CHECK(rep3.errors == 0);
    CHECK(get(w / "x" / "CHARS" / "MODELS" / "HERO.ABC") == "model data");
    CHECK(get(w / "x" / "WORLDS" / "DM_TEST.DAT") == std::string(1000, 'x'));
    CHECK(get(w / "x" / "NOEXT") == "no extension");
    CHECK(fs::exists(w / "x" / "EMPTY.WAV"));

    // Editing as Rez Studio does, then saving over the open file. (Windows
    // won't replace a file another handle has open: close the others.)
    a.reset();
    b.reset();
    d.reset();
    {
        auto e = rez::Archive::open(rz.string());
        put(w / "new" / "added.txt", "added");
        rez::Report er;
        rez::CreateOptions eo;
        eo.time = 1100000000;
        auto* skins = e->root().find_dir_path("CHARS/SKINS");
        CHECK(rez::add_file(*e, *skins, (w / "new" / "added.txt").string(), eo, er) != nullptr);
        // rename HERO_HEAD? (none here) -> rename HERO.DTX to MAIN.DTX
        auto* hero = e->root().find_path("CHARS/SKINS/HERO.DTX");
        auto owned = hero->parent()->take(hero);
        owned->name = "MAIN";
        skins->add(std::move(owned));
        // move WORLDS under CHARS
        auto* worlds = e->root().find_dir("WORLDS");
        auto wd = e->root().take_dir(worlds);
        e->root().find_dir("CHARS")->adopt(std::move(wd));
        // delete NOEXT
        auto* noext = e->root().find_path("NOEXT");
        e->root().take(noext);
        // import from another archive
        put(w / "other" / "sounds" / "boom.wav", "boom!");
        rez::Report or_;
        auto other = rez::create_from_directory((w / "other").string(), eo, or_);
        other->save((w / "other.rez").string());
        auto o2 = rez::Archive::open((w / "other.rez").string());
        auto* boom = o2->root().find_path("SOUNDS/BOOM.WAV");
        auto* snd = e->root().add_dir("SOUNDS", 1100000000);
        auto c = rez::Resource::copy_of(*boom);
        c->data_order = e->next_data_order();
        snd->add(std::move(c));
        e->save(rz.string());   // over itself
        auto f = rez::Archive::open(rz.string());
        auto txt = [&](const char* p) {
            auto* r = f->root().find_path(p);
            if (!r) return std::string("<missing>");
            auto v = r->read();
            return std::string(v.begin(), v.end());
        };
        CHECK(txt("CHARS/SKINS/ADDED.TXT") == "added");
        CHECK(txt("CHARS/SKINS/MAIN.DTX") == "skin");
        CHECK(f->root().find_path("CHARS/SKINS/HERO.DTX") == nullptr);
        CHECK(txt("CHARS/WORLDS/DM_TEST.DAT") == std::string(1000, 'x'));
        CHECK(f->root().find_path("NOEXT") == nullptr);
        CHECK(txt("SOUNDS/BOOM.WAV") == "boom!");
        CHECK(txt("CHARS/MODELS/HERO.ABC") == "model data");
        // and the edited archive is still readable by the old file handle's successor
        CHECK(e->root().find_path("CHARS/SKINS/MAIN.DTX") != nullptr);
        auto v = e->root().find_path("CHARS/SKINS/MAIN.DTX")->read();
        CHECK(std::string(v.begin(), v.end()) == "skin");
        // compaction keeps everything
        f->compact_order();
        f->save((w / "compact.rez").string());
        auto g = rez::Archive::open((w / "compact.rez").string());
        CHECK(g->root().count_resources() == f->root().count_resources());
    }

    std::printf("%s (%d failure(s))\n", fails ? "FAILED" : "ok", fails);
    return fails ? 1 : 0;
}
