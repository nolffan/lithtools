// A small PS2 archive made here, then read and edited through rez::Archive:
// names from the dictionary, a file replaced in place and its identical copy
// in another table with it, a file replaced by bigger data (appended) whose
// variant in another table stays, a new file whose name the archive keeps,
// a rename that keeps each table's entry, a removal, the table capacity,
// and a rebuild from the tables that comes out byte-identical.
//   ps2rez_test <work dir>
#include <lithtech/ps2rez.hpp>
#include <lithtech/rez.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace lithtech;
namespace fs = std::filesystem;

static int failures = 0;
static void check(bool ok, const char* what) {
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) ++failures;
}

static void put32(std::vector<uint8_t>& b, std::size_t at, uint32_t v) {
    for (int i = 0; i < 4; ++i) b[at + i] = uint8_t(v >> (8 * i));
}

static std::vector<uint8_t> bytes(const std::string& s) { return {s.begin(), s.end()}; }

static std::vector<uint8_t> slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    fs::create_directories(argv[1]);
    const std::string path = (fs::path(argv[1]) / "test_ps2.rez").string();

    // Three files in three tables. Table 1 has an identical copy of A,
    // table 2 a variant of B; both have the other files where table 0 has.
    auto& names = ps2rez::Names::global();
    names.add("DIR\\A.TXT");
    names.add("DIR\\B.DTX");
    names.add("OTHER\\Z.TXT");
    const uint32_t hA = ps2rez::hash("DIR\\A.TXT"), hB = ps2rez::hash("dir/b.dtx"), hZ = ps2rez::hash("OTHER\\Z.TXT");
    check(hB == ps2rez::hash("DIR\\B.DTX"), "the hash ignores case and slash direction");
    const uint32_t n = 3, ntab = 3, stride = 2048, data = 1 + ntab;   // 3 entries fit one sector
    std::vector<uint8_t> f(uint64_t(data + 8) * 2048, 0);
    put32(f, 0, 3), put32(f, 4, n), put32(f, 8, 10), put32(f, 12, data), put32(f, 16, ntab);
    auto blob = [&](uint32_t sector, const std::string& s) { std::memcpy(&f[uint64_t(sector) * 2048], s.data(), s.size()); };
    const std::string A = std::string(3000, 'a'), B = "B version 0", B2 = "B variant", Z = "zzz";
    blob(data, A);           // A: sectors data, data+1
    blob(data + 2, B);
    blob(data + 3, Z);
    blob(data + 4, A);       // table 1's copy of A
    blob(data + 6, B2);      // table 2's variant of B
    struct E { uint32_t h, s, z; };
    const E t0[] = {{hA, data, uint32_t(A.size())}, {hB, data + 2, uint32_t(B.size())}, {hZ, data + 3, 3}};
    const E t1[] = {{hA, data + 4, uint32_t(A.size())}, {hB, data + 2, uint32_t(B.size())}, {hZ, data + 3, 3}};
    const E t2[] = {{hA, data, uint32_t(A.size())}, {hB, data + 6, uint32_t(B2.size())}, {hZ, data + 3, 3}};
    const E* tabs[] = {t0, t1, t2};
    for (uint32_t t = 0; t < ntab; ++t)
        for (uint32_t i = 0; i < n; ++i) {
            std::size_t at = 2048 + t * stride + 16 * i;
            put32(f, at, tabs[t][i].h), put32(f, at + 4, tabs[t][i].s), put32(f, at + 8, tabs[t][i].z);
            put32(f, at + 12, (tabs[t][i].z + 2047) / 2048);
        }
    { std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(f.data()), std::streamsize(f.size())); }

    check(ps2rez::probe(path) && rez::Archive::probe(path), "probe recognises it");
    {
        auto p = ps2rez::Archive::open(path);
        p->write_rebuilt(path + ".rebuilt");
        check(slurp(path + ".rebuilt") == f, "rebuilt from its tables, byte-identical");
    }
    auto ar = rez::Archive::open(path);
    check(ar->is_ps2(), "opens as a PS2 archive");
    rez::Resource* ra = ar->root().find_path("DIR\\A.TXT");
    rez::Resource* rb = ar->root().find_path("dir/b.dtx");
    check(ra && rb && ar->root().find_path("OTHER/Z.TXT"), "files are named from the dictionary");
    check(ra && ra->read() == bytes(A) && ra->id == hA, "reads table 0's data; the ID is the hash");

    // replace A in place (fits) and B with bigger data (appended)
    auto mem = [](rez::Resource* r, const std::string& s) {
        r->source = rez::DataSource{};
        r->source.kind = rez::DataSource::Kind::Memory;
        r->source.bytes = std::make_shared<const std::vector<uint8_t>>(s.begin(), s.end());
        r->size = uint32_t(s.size());
    };
    const std::string A2 = "short A", Bbig = std::string(5000, 'B');
    mem(ra, A2);
    mem(rb, Bbig);
    // a new file whose name the dictionary doesn't have
    rez::Directory* nd = ar->root().add_dir("NEW");
    auto rc = std::make_unique<rez::Resource>();
    rc->name = "C";
    rc->type = rez::type_from_string("TXT");
    nd->add(std::move(rc));
    mem(ar->root().find_path("NEW\\C.TXT"), "new file");
    ar->save(path);
    {
        auto p = ps2rez::Archive::open(path);
        auto in = [&](uint32_t t, uint32_t h) {
            for (auto& e : p->tables[t])
                if (e.hash == h) return e;
            return ps2rez::Entry{};
        };
        check(p->read(in(0, hA)) == bytes(A2) && p->read(in(1, hA)) == bytes(A2) && in(0, hA).sector == data &&
                  in(1, hA).sector == data + 4,
              "A replaced in place: table 0's and table 1's identical copy");
        check(p->read(in(0, hB)) == bytes(Bbig) && in(0, hB).sector >= data + 8, "B replaced by bigger data, appended");
        check(p->read(in(2, hB)) == bytes(B2), "table 2's variant of B stays");
        check(p->header.entries == 5, "the new file and the names file are in every table");
    }
    ar.reset();
    names = ps2rez::Names{};   // forget everything: only the archive's own names file can name C now
    ps2rez::Names::global().add("DIR\\A.TXT");
    ar = rez::Archive::open(path);
    check(ar->root().find_path("NEW/C.TXT") && ar->root().find_path("NEW/C.TXT")->read() == bytes("new file"),
          "the added file's name comes back from LITHTOOLS\\NAMES.TXT");

    // rename A (each table keeps its own entry), remove B
    rez::Resource* a2 = ar->root().find_path("DIR\\A.TXT");
    auto moved = a2->parent()->take(a2);
    moved->name = "RENAMED";
    nd = ar->root().find_dir("NEW");
    nd->add(std::move(moved));
    // B lost its name with the dictionary: _UNNAMED\<hash>.<ext>
    if (rez::Directory* un = ar->root().find_dir("_UNNAMED"))
        for (auto& r : un->resources())
            if (r->id == hB) {
                un->take(r.get());
                break;
            }
    ar->save(path);
    {
        auto p = ps2rez::Archive::open(path);
        uint32_t hR = ps2rez::hash("NEW\\RENAMED.TXT");
        auto in = [&](uint32_t t, uint32_t h) -> const ps2rez::Entry* {
            for (auto& e : p->tables[t])
                if (e.hash == h) return &e;
            return nullptr;
        };
        check(!in(0, hA) && in(0, hR) && in(0, hR)->sector == data && in(1, hR)->sector == data + 4,
              "the rename keeps each table's entry under the new name");
        check(!in(0, hB) && !in(2, hB), "B is gone from every table");
        check(ar->root().find_path("NEW/RENAMED.TXT")->read() == bytes(A2), "the tree reads the result");
    }

    // too many files for the tables' sector
    for (int i = 0; i < 200; ++i) {
        auto r = std::make_unique<rez::Resource>();
        r->name = "F" + std::to_string(i);
        r->type = rez::type_from_string("TXT");
        rez::Resource* x = nd->add(std::move(r));
        mem(x, "x");
    }
    bool refused = false;
    try {
        ar->save(path + ".full");
    } catch (const std::exception& e) {
        refused = std::strstr(e.what(), "at most") != nullptr;
    }
    check(refused && !fs::exists(path + ".full"), "more files than the tables hold is refused before writing");

    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}
