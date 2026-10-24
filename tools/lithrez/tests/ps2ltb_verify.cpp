// Reads every PS2 model (.ltb, file type 2) under a directory with
// lithtech::ps2ltb::read and checks it: the model converts for drawing
// (ps2ltb::to_abc), its triangles are the header's count, bind-pose
// skinning gives back the model-space positions, every value is finite,
// and the first animation's first key is near the bind pose where it is
// the base pose. Other .ltb files are skipped.
//   ps2ltb_verify <dir>
//   ps2ltb_verify --dump <file.ltb>   values to compare with ps2ltb.py
//                                     (tests/ps2ltb_dump.py prints the same)
#include <lithtech/ps2ltb.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;
using namespace lithtech;

static std::vector<uint8_t> load(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static int dump(const char* path) {
    ps2ltb::Model m = ps2ltb::read(load(path));
    abc::Model a = ps2ltb::to_abc(m);
    std::printf("nodes %zu pieces %zu anims %zu sockets %zu children %zu mult %u\n", m.nodes.size(), m.pieces.size(),
                m.animations.size(), m.sockets.size(), m.child_models.size(), m.hash_mult);
    for (std::size_t pi = 0; pi < a.pieces.size(); ++pi) {
        std::printf("piece %zu %s tex %u\n", pi, a.pieces[pi].name.c_str(), a.pieces[pi].texture_index);
        for (std::size_t li = 0; li < a.pieces[pi].lods.size(); ++li) {
            const auto& l = a.pieces[pi].lods[li];
            std::printf(" lod %zu kind %d verts %zu tris %zu\n", li, m.pieces[pi].lods[li].kind, l.vertices.size(),
                        l.triangles.size());
            for (std::size_t vi = 0; vi < l.vertices.size() && vi < 3; ++vi) {
                const auto& v = l.vertices[vi];
                std::printf("  v%zu %.6f %.6f %.6f n %.6f %.6f %.6f", vi, v.location[0], v.location[1], v.location[2],
                            v.normal[0], v.normal[1], v.normal[2]);
                for (auto& w : v.weights) std::printf(" %u:%.6f", w.node, w.bias);
                std::printf("\n");
            }
            for (std::size_t ti = 0; ti < l.triangles.size() && ti < 2; ++ti) {
                const auto& t = l.triangles[ti];
                std::printf("  t%zu %u %u %u uv %.6f %.6f\n", ti, t.indices[0], t.indices[1], t.indices[2], t.uv[0][0],
                            t.uv[0][1]);
            }
        }
    }
    for (std::size_t ai = 0; ai < a.animations.size() && ai < 3; ++ai) {
        const auto& an = a.animations[ai];
        std::printf("anim %zu %s keys %zu\n", ai, an.name.c_str(), an.keyframes.size());
        for (std::size_t n = 0; n < a.nodes.size() && n < 4; ++n)
            for (std::size_t k = 0; k < an.keyframes.size() && k < 2; ++k) {
                auto [p, q] = an.transforms[n][k];
                std::printf("  n%zu k%zu %.6f %.6f %.6f %.6f %.6f %.6f %.6f\n", n, k, p[0], p[1], p[2], q[0], q[1], q[2],
                            q[3]);
            }
    }
    for (auto& s : a.sockets) std::printf("socket %s node %u\n", s.name.c_str(), s.node);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--dump") return dump(argv[2]);
    if (argc < 2) {
        std::printf("usage: ps2ltb_verify <dir> | --dump <file>\n");
        return 2;
    }
    int files = 0, ok = 0, other = 0, failed = 0, nonfinite = 0, tri_match = 0;
    std::map<int, long> kinds;
    long checked = 0, skin_ok = 0, named = 0, names = 0;
    double worst_skin = 0;
    int pose_models = 0, pose_match = 0;
    for (auto& e : fs::recursive_directory_iterator(argv[1])) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        for (auto& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".ltb") continue;
        auto d = load(e.path());
        if (!ps2ltb::is_ps2_model(d)) {
            ++other;
            continue;
        }
        ++files;
        ps2ltb::Model m;
        abc::Model a;
        try {
            m = ps2ltb::read(d);
            a = ps2ltb::to_abc(m);
        } catch (const std::exception& ex) {
            std::printf("FAIL %s: %s\n", e.path().u8string().c_str(), ex.what());
            ++failed;
            continue;
        }
        ++ok;
        std::size_t tris = 0;
        for (auto& p : m.pieces)
            for (auto& l : p.lods) {
                kinds[l.kind]++;
                tris += l.triangle_count();
            }
        if (tris == m.counts[5]) ++tri_match;
        for (auto& p : a.pieces) {
            ++names;
            named += p.name[0] != '#';
        }
        for (auto& an : a.animations) {
            ++names;
            named += an.name[0] != '#';
        }
        auto bind = abc::bind_pose(a);
        bool bad = false;
        for (auto& p : a.pieces)
            for (auto& l : p.lods)
                for (auto& v : l.vertices) {
                    for (float x : v.location)
                        if (!std::isfinite(x)) bad = true;
                    if (v.weights.empty()) continue;
                    float sum = 0;
                    for (auto& w : v.weights) sum += w.bias;
                    if (std::fabs(sum - 1) > 1e-3f) continue;
                    ++checked;
                    auto s = abc::skin(v, bind);
                    float dd = 0;
                    for (int i = 0; i < 3; ++i) dd = std::max(dd, std::fabs(s[i] - v.location[i]));
                    worst_skin = std::max(worst_skin, double(dd));
                    if (dd < 1e-2f) ++skin_ok;
                }
        if (!a.animations.empty() && !a.animations[0].keyframes.empty()) {
            auto P = abc::pose(a, a.animations[0], float(a.animations[0].keyframes[0].time));
            double mw = 0;
            for (std::size_t n = 0; n < P.size(); ++n)
                for (int i = 0; i < 12; ++i) {
                    if (!std::isfinite(P[n][i])) bad = true;
                    double dv = std::fabs(P[n][i] - a.nodes[n].bind[i]);
                    if (i % 4 == 3) dv /= std::max(1.0f, a.internal_radius);
                    mw = std::max(mw, dv);
                }
            ++pose_models;
            if (mw < 2e-3) ++pose_match;
        }
        if (bad) {
            ++nonfinite;
            std::printf("NONFINITE %s\n", e.path().u8string().c_str());
        }
    }
    std::printf("%d PS2 models: %d read, %d failed (%d other .ltb files skipped)\n", files, ok, failed, other);
    std::printf("meshes by kind (4 rigid, 5 skeletal):");
    for (auto [k, n] : kinds) std::printf(" %d:%ld", k, n);
    std::printf("\ntriangles equal the header's count in %d of %d models\n", tri_match, ok);
    std::printf("piece and animation names resolved: %ld of %ld\n", named, names);
    std::printf("bind skinning: %ld of %ld weighted vertices back within 0.01 (worst %.3g)\n", skin_ok, checked,
                worst_skin);
    std::printf("first key of the first animation equals the bind pose in %d of %d models\n", pose_match, pose_models);
    std::printf("models with non-finite values: %d\n", nonfinite);
    return failed ? 1 : 0;
}
