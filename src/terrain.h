#ifndef VOXEL_ENGINE_TERRAIN_H
#define VOXEL_ENGINE_TERRAIN_H

#include <glm/glm.hpp>

constexpr int ATLAS_TILES = 16;
constexpr float TILE = 1.0f/ATLAS_TILES;//size of each tile
constexpr int CX = 16, CY = 128, CZ = 16;

static const glm::ivec3 DIR[6] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

//+X, -X, +Y, -Y, +Z, -Z
static const float FACE[6][4][3] = {
    {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},
    {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
    {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
    {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
    {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
    {{1,0,0},{0,0,0},{0,1,0},{1,1,0}},
};

enum BlockType {
    AIR,
    STONE,
    DIRT,
    GRASS,
};

struct UVRect {
    float u0, v0, u1, v1;
};

struct BlockInfo {
    bool solid;
    int tex[6][2];//per face x,y texcoords
};

struct Chunk {

};

inline UVRect tileUV(int tx, int ty) {
    float u0 = tx * TILE;
    float v1 = 1.0f - ty * TILE;
    return { u0, v1 - TILE, u0 + TILE, v1 };
}

inline BlockInfo& blockInfo(BlockType t) {
    static BlockInfo infos[] = {
        //                           side     side     top    bottom     side    side
        { false, {} },//air
        { true,  {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}} },//stone
        { true,  {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}} },//dirt
        { true,  {{3,0},{3,0},{0,0},{2,0},{3,0},{3,0}} },//grass
    };
    return infos[t];
}

inline void addFace(std::vector<float>& verts, std::vector<unsigned int>& idx, int face, glm::vec3 base, int tx, int ty, glm::vec3 tint = {1,1,1}) {
    UVRect uv = tileUV(tx, ty);

    float u[4] = {uv.u0, uv.u1, uv.u1, uv.u0};
    float v[4] = {uv.v0, uv.v0, uv.v1, uv.v1};

    unsigned int start = verts.size()/8;
    for (int i=0; i<4; i++) {
        verts.push_back(base.x+FACE[face][i][0]);
        verts.push_back(base.y+FACE[face][i][1]);
        verts.push_back(base.z+FACE[face][i][2]);
        verts.push_back(u[i]);
        verts.push_back(v[i]);
        verts.push_back(tint.r);
        verts.push_back(tint.g);
        verts.push_back(tint.b);
    }
    idx.insert(idx.end(), { start, start+1, start+2, start+2, start+3, start });
}

//temp
inline void buildBlockMesh(BlockType t, std::vector<float>& verts, std::vector<unsigned int>& idx) {
    BlockInfo& info = blockInfo(t);
    for (int f = 0; f < 6; f++) {
        glm::vec3 tint = {1,1,1};
        if (t == GRASS && f == 2) tint = {0.65f, 0.95f, 0.50f};
        addFace(verts, idx, f, {0,0,0}, info.tex[f][0], info.tex[f][1], tint);
    }
}

class terrain {

};


#endif