// LithTech .ltb models (Jupiter: NOLF2, Contract J.A.C.K., Tron 2.0):
// reading, for previews. The C++ twin of plugins/blender's lithtech/ltb.py,
// which reads and writes back every shipped .ltb byte for byte;
// plugins/blender/docs/LTB.md has the format and the evidence.
//
// D3D models only (LTB file type 1, LTB version 9, model file versions 18
// and 20 to 25). The chunked leftovers some FX folders ship, which the
// games refuse, are refused here too.

#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "lithtech/abc.hpp"

namespace lithtech::ltb {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

using Vec2 = std::array<float, 2>;
using Vec3 = std::array<float, 3>;
using Quat = std::array<float, 4>;      // x, y, z, w
using Mat4 = std::array<float, 16>;     // row-major, translation in column 3

// CRenderObject::RENDER_OBJECT_TYPES
enum MeshKind { RIGID_MESH = 4, SKEL_MESH = 5, VA_MESH = 6, NULL_MESH = 7 };

// MODEL_ANIM_COMPRESSION_TYPES
enum Compression { ANCMPRS_NONE = 0, ANCMPRS_REL = 1, ANCMPRS_REL_16 = 2, ANCMPRS_REL_16_ROT_ONLY = 3 };

struct Vertex {
    Vec3 position{};
    std::vector<float> weights;        // D3D blend weights: one fewer than the bones blended
    std::array<uint8_t, 4> bone_indices{};   // matrix-palette meshes
    Vec3 normal{};
    std::vector<Vec2> uvs;             // per UV set; v down
};

struct BoneSet {
    uint16_t first_vertex = 0, vertex_count = 0;
    std::array<uint8_t, 4> bones{};    // 0xFF unused
    uint32_t index_into_index_buffer = 0;
};

struct Mesh {
    int kind = NULL_MESH;
    uint32_t vertex_count = 0, poly_count = 0;
    uint32_t max_bones_per_tri = 1, max_bones_per_vert = 1;
    std::array<uint32_t, 4> stream_flags{};
    uint32_t bone_effector = 0;        // rigid, VA: the node it follows
    bool reindexed_bones_flag = false, use_matrix_palettes = false;
    uint32_t min_bone = 0, max_bone = 0;
    std::vector<uint32_t> reindexed_bones;
    uint32_t undup_vertex_count = 0, anim_node_index = 0;   // VA
    int basis_floats = 6;
    bool raw_vertices = false;         // layout unknown (0-bone skeletal); positions only
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;     // 3 per triangle
    std::vector<BoneSet> bone_sets;
    std::vector<std::pair<uint16_t, uint16_t>> dup_map;
};

struct LOD {
    uint32_t texture_count = 0;
    std::array<int32_t, 4> textures{{-1, -1, -1, -1}};   // skin slots
    int32_t render_style = 0;
    uint8_t render_priority = 0;
    Mesh mesh;
    std::vector<uint8_t> used_nodes;
};

struct Piece {
    std::string name;
    std::vector<float> lod_distances;  // one per LOD
    std::vector<LOD> lods;
};

struct Node {
    std::string name;
    uint16_t index = 0;
    uint8_t flags = 0;
    Mat4 bind{};                       // global transform at the bind pose
    int parent = -1;
};

struct Keyframe {
    uint32_t time = 0;
    std::string command;
};

// One node's keys as stored, decoded to floats (16-bit encodings scaled,
// not yet normalised).
struct Track {
    bool vertex_anim = false;
    std::vector<std::vector<float>> vertex_frames;
    std::vector<Vec3> positions;
    std::vector<Quat> rotations;
    std::vector<uint32_t> pos_times, rot_times;   // versions 18 and 20 (REL)
};

struct Animation {
    std::string name;
    Vec3 dims{};
    uint32_t compression = ANCMPRS_NONE;
    uint32_t interpolation_ms = 200;
    std::vector<Keyframe> keyframes;
    std::vector<Track> tracks;         // Model::nodes order

    // A node's (position, rotation) at a keyframe, as the engine's channel
    // interpreters decode it (see ltb.py, Animation.key_transform).
    std::pair<Vec3, Quat> key_transform(std::size_t node, std::size_t key) const;
};

struct Socket {
    uint32_t node = 0;
    std::string name;
    Quat rotation{};
    Vec3 position{}, scale{1, 1, 1};
};

struct Model {
    uint32_t version = 0;              // model file version
    std::string command_string;
    float radius = 0;
    std::vector<Piece> pieces;
    std::vector<Node> nodes;           // depth-first, parent first
    std::vector<std::string> child_models;
    std::vector<Animation> animations;
    std::vector<Socket> sockets;
};

// Whether data starts like an .ltb (D3D or chunked).
bool is_ltb(const std::vector<uint8_t>& data);
bool is_chunked(const std::vector<uint8_t>& data);

Model read(const std::vector<uint8_t>& data);

// The (node, weight) pairs each vertex of a mesh is blended with, as the D3D
// renderer does it (bone sets stop at the first 0xFF; the last bone takes 1
// minus the others).
std::vector<std::vector<std::pair<uint32_t, float>>> vertex_weights(const Mesh& m);

// The model as an abc::Model, for code that draws those (pieces' LODs as
// triangles with per-corner UVs, weights relative to their node's bind
// matrix, animations as per-key transforms).
abc::Model to_abc(const Model& m);

}  // namespace lithtech::ltb
