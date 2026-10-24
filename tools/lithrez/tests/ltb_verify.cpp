// Reads every .ltb under a directory with lithtech::ltb::read and checks it:
// the model converts for drawing (ltb::to_abc), bind-pose skinning gives
// back the stored positions (vertices are stored in model space at bind),
// every value is finite, and the first animation's first key is near the
// bind pose where it is the base pose.
//   ltb_verify <dir>
//   ltb_verify --dump <file.ltb>   values to compare with ltb.py's reader
#include <lithtech/ltb.hpp>

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
    ltb::Model m = ltb::read(load(path));
    std::printf("version %u nodes %zu pieces %zu anims %zu sockets %zu children %zu\n", m.version, m.nodes.size(),
                m.pieces.size(), m.animations.size(), m.sockets.size(), m.child_models.size());
    for (std::size_t pi = 0; pi < m.pieces.size(); ++pi)
        for (std::size_t li = 0; li < m.pieces[pi].lods.size(); ++li) {
            const auto& me = m.pieces[pi].lods[li].mesh;
            std::printf("piece %zu lod %zu kind %d verts %zu tris %zu sets %zu\n", pi, li, me.kind, me.vertices.size(),
                        me.indices.size() / 3, me.bone_sets.size());
            for (std::size_t vi = 0; vi < me.vertices.size() && vi < 3; ++vi) {
                const auto& v = me.vertices[vi];
                std::printf("  v%zu %.9g %.9g %.9g n %.9g %.9g %.9g", vi, v.position[0], v.position[1], v.position[2],
                            v.normal[0], v.normal[1], v.normal[2]);
                for (float w : v.weights) std::printf(" w %.9g", w);
                if (!v.uvs.empty()) std::printf(" uv %.9g %.9g", v.uvs[0][0], v.uvs[0][1]);
                std::printf("\n");
            }
            auto ws = ltb::vertex_weights(me);
            for (std::size_t vi = 0; vi < ws.size() && vi < 2; ++vi) {
                std::printf("  vw%zu", vi);
                for (auto [n, w] : ws[vi]) std::printf(" %u:%.6f", n, w);
                std::printf("\n");
            }
        }
    for (std::size_t ai = 0; ai < m.animations.size() && ai < 3; ++ai) {
        const auto& a = m.animations[ai];
        std::printf("anim %zu %s comp %u keys %zu\n", ai, a.name.c_str(), a.compression, a.keyframes.size());
        for (std::size_t n = 0; n < m.nodes.size() && n < 4; ++n)
            for (std::size_t k = 0; k < a.keyframes.size() && k < 3; ++k) {
                auto [p, q] = a.key_transform(n, k);
                std::printf("  n%zu k%zu %.6f %.6f %.6f %.6f %.6f %.6f %.6f\n", n, k, p[0], p[1], p[2], q[0], q[1],
                            q[2], q[3]);
            }
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--dump") return dump(argv[2]);
    if (argc < 2) {
        std::printf("usage: ltb_verify <dir> | --dump <file>\n");
        return 2;
    }
    int files = 0, ok = 0, chunked = 0, failed = 0, nonfinite = 0;
    std::map<uint32_t, int> versions;
    std::map<int, long> kinds;
    long checked = 0, skin_ok = 0;
    double worst_skin = 0;
    int pose_models = 0, pose_match = 0;
    for (auto& e : fs::recursive_directory_iterator(argv[1])) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        for (auto& c : ext) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".ltb") continue;
        ++files;
        auto d = load(e.path());
        if (ltb::is_chunked(d)) {
            ++chunked;
            try {
                ltb::read(d);
                std::printf("FAIL %s: chunked file not refused\n", e.path().u8string().c_str());
                ++failed;
            } catch (const ltb::Error&) {
            }
            continue;
        }
        ltb::Model m;
        abc::Model a;
        try {
            m = ltb::read(d);
            a = ltb::to_abc(m);
        } catch (const std::exception& ex) {
            std::printf("FAIL %s: %s\n", e.path().u8string().c_str(), ex.what());
            ++failed;
            continue;
        }
        ++ok;
        versions[m.version]++;
        for (auto& p : m.pieces)
            for (auto& l : p.lods) kinds[l.mesh.kind]++;
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
            if (mw < 1e-3) ++pose_match;
        }
        if (bad) {
            ++nonfinite;
            std::printf("NONFINITE %s\n", e.path().u8string().c_str());
        }
    }
    std::printf("%d files: %d read, %d chunked (refused, as the engine does), %d failed\n", files, ok, chunked, failed);
    std::printf("versions:");
    for (auto [v, n] : versions) std::printf(" %u:%d", v, n);
    std::printf("\nmeshes by kind (4 rigid, 5 skeletal, 6 vertex-animated, 7 null):");
    for (auto [k, n] : kinds) std::printf(" %d:%ld", k, n);
    std::printf("\nbind skinning: %ld of %ld weighted vertices back within 0.01 (worst %.3g)\n", skin_ok, checked,
                worst_skin);
    std::printf("first key of the first animation equals the bind pose in %d of %d models\n", pose_match, pose_models);
    std::printf("models with non-finite values: %d\n", nonfinite);
    return failed ? 1 : 0;
}
