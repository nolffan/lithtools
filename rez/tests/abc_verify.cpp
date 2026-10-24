// Reads every .abc under a directory with lithtech::abc::read and checks the
// skinning maths: skin(bind) gives back the stored position for vertices
// whose biases sum to 1, and pose() at the first key of the first animation
// gives back the bind matrices.
#include <lithtech/abc.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;
using namespace lithtech;

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: abc_verify <dir>\n"); return 2; }
    int files = 0, failed = 0;
    long verts = 0, skin_ok = 0, skin_checked = 0;
    double worst_pose = 0;
    int pose_models = 0, pose_match = 0;
    for (auto& e : fs::recursive_directory_iterator(argv[1])) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        for (auto& c : ext) c = char(std::tolower(c));
        if (ext != ".abc") continue;
        ++files;
        std::ifstream f(e.path(), std::ios::binary);
        std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        abc::Model m;
        try {
            m = abc::read(d);
        } catch (const std::exception& ex) {
            std::printf("FAIL %s: %s\n", e.path().c_str(), ex.what());
            ++failed;
            continue;
        }
        auto bind = abc::bind_pose(m);
        for (auto& p : m.pieces)
            for (auto& v : p.lods[0].vertices) {
                ++verts;
                float sum = 0;
                for (auto& w : v.weights) sum += w.bias;
                if (std::fabs(sum - 1) > 1e-4f) continue;
                ++skin_checked;
                auto s = abc::skin(v, bind);
                float dd = 0;
                for (int i = 0; i < 3; ++i) dd = std::max(dd, std::fabs(s[i] - v.location[i]));
                if (dd < 1e-2f) ++skin_ok;
            }
        if (!m.animations.empty() && !m.animations[0].keyframes.empty()) {
            auto P = abc::pose(m, m.animations[0], float(m.animations[0].keyframes[0].time));
            double mw = 0;
            for (std::size_t n = 0; n < P.size(); ++n)
                for (int i = 0; i < 12; ++i) {
                    double dv = std::fabs(P[n][i] - m.nodes[n].bind[i]);
                    // translation scaled by model size
                    if (i % 4 == 3) dv /= std::max(1.0f, m.internal_radius);
                    worst_pose = std::max(worst_pose, dv);
                    mw = std::max(mw, dv);
                }
            ++pose_models;
            if (mw < 1e-3) ++pose_match;
        }
    }
    std::printf("%d files, %d failed; %ld vertices, %ld with biases summing to 1, %ld of them skin back "
                "within 0.01 (%.2f%%); first key of the first animation equals the bind pose in %d of %d models (worst %.4g)\n",
                files, failed, verts, skin_checked, skin_ok, skin_checked ? 100.0 * skin_ok / skin_checked : 0.0,
                pose_match, pose_models, worst_pose);
    return failed ? 1 : 0;
}
