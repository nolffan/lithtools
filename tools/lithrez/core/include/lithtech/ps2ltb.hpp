// PS2 NOLF models (.ltb, file type 2, version 16): reading, for previews.
// The C++ twin of plugins/blender's lithtech/ps2ltb.py, which reads and
// writes back every model on both PS2 discs byte for byte;
// plugins/blender/docs/PS2LTB.md has the format.
//
// Geometry is the PS2's: per LOD a DMA chain of VU buffers holding triangle
// strips (position, normal, s t q). Rigid meshes keep their vertices in
// their node's space; skeletal ones in model space, with unique vertices
// carrying up to four weights. Piece, animation and socket names are only
// hashes; names() looks them up in the dictionary the Blender extension
// ships (ps2ltb_names.txt, compiled in).

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "lithtech/abc.hpp"

namespace lithtech::ps2ltb {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

using Vec3 = std::array<float, 3>;
using Quat = std::array<float, 4>;      // x, y, z, w
using Mat4 = std::array<float, 16>;     // row-major, translation in column 3

enum MeshKind { RIGID_MESH = 4, SKEL_MESH = 5 };

struct StripVertex {
    std::array<float, 4> position{};   // x y z 1 (rigid: node space; skeletal: model space)
    std::array<float, 4> normal{};
    std::array<float, 3> stq{};        // s t q
    uint32_t extra = 0;
    int unique = -1;                   // skeletal: the unique vertex it copies
};

struct Strip {
    bool reversed = false;             // the first triangle goes the other way
    std::vector<StripVertex> vertices;
    // (i, j, k) into vertices, PC (clockwise) order
    std::vector<std::array<int, 3>> triangles() const;
};

struct UniqueVertex {
    std::array<float, 4> position{}, normal{};
    std::array<int16_t, 4> weights{};  // 4096 = 1.0
    std::array<uint8_t, 4> bones{};    // local bone * 4, 0xff unused
};

struct LOD {
    int kind = RIGID_MESH;
    uint32_t node = 0;                 // rigid: the node it hangs on
    std::vector<Strip> strips;
    std::vector<UniqueVertex> uniques;
    std::vector<uint32_t> bone_map;    // local bone -> node
    std::size_t triangle_count() const;
};

struct Piece {
    uint32_t name_hash = 0;
    uint32_t texture = 0;
    std::vector<LOD> lods;
};

struct Node {
    std::string name;
    Mat4 bind{};
    uint32_t name_hash = 0, child_count = 0;
    uint16_t index = 0, flags = 0;
    int parent = -1;
};

struct Keyframe {
    uint32_t time = 0;
    std::string command;
};

struct Animation {
    Vec3 dims{}, translation{};
    uint32_t name_hash = 0, interpolation_ms = 200;
    std::vector<Keyframe> keyframes;
    // keys[node][key]: x y z flag qx qy qz qw (s16)
    std::vector<std::vector<std::array<int16_t, 8>>> keys;
    std::pair<Vec3, Quat> key_transform(std::size_t node, std::size_t key) const;
};

struct Socket {
    Quat rotation{};
    Vec3 position{};
    uint32_t node = 0, name_hash = 0, name_hash1 = 0;
};

struct Model {
    std::array<uint32_t, 14> counts{};
    std::string command;
    float radius = 0;
    uint32_t hash_mult = 1;
    std::vector<float> lod_distances;
    std::vector<Piece> pieces;
    std::vector<Node> nodes;           // depth-first, parent first
    std::vector<std::string> child_models;
    std::vector<Animation> animations;
    std::vector<Socket> sockets;

    std::string piece_name(const Piece& p) const;
    std::string animation_name(const Animation& a) const;
    std::string socket_name(const Socket& s) const;
};

// The hash a PS2 model stores for a name (upper case, '/' as '\'):
// h = h + c + h * mult.
uint32_t name_hash(const std::string& name, uint32_t mult);

// A name for a hash from the compiled-in dictionary (earlier entries win a
// collision), or none.
std::optional<std::string> lookup(uint32_t hash, uint32_t mult);

// Whether data starts as a PS2 model (file type 2, version 16).
bool is_ps2_model(const std::vector<uint8_t>& data);

Model read(const std::vector<uint8_t>& data);

// The model as an abc::Model, for code that draws those: rigid strip
// vertices told apart by position and normal and moved into model space,
// skeletal meshes by their unique vertices; weights relative to their
// node's bind matrix; animations as per-key transforms.
abc::Model to_abc(const Model& m);

}  // namespace lithtech::ps2ltb
