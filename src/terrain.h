#ifndef VOXEL_ENGINE_TERRAIN_H
#define VOXEL_ENGINE_TERRAIN_H

#include <bit>
#include <memory>
#include <glm/glm.hpp>

#include "worldgen.h"

class engine;

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
    SAND,
    WATER,
};

struct UVRect {
    float u0, v0, u1, v1;
};

struct BlockInfo {
    bool solid;
    bool opaque; //water, glass
    float alpha;
    int tex[6][2];//per face x,y texcoords
};

struct Chunk {
    glm::ivec2 coord;
    uint8_t blocks[CX * CY * CZ]={};

    bool dirty = false;
    int renderId = -1;
    int waterRenderId = -1;

    static bool inBounds(int x, int y, int z) {
        return x >= 0 && x < CX && y >= 0 && y < CY && z >= 0 && z < CZ;
    }

    uint8_t get(int x, int y, int z) {
        if (!inBounds(x, y, z)) return AIR;
        return blocks[x + z*CX + y*CX*CZ];
    }
};

inline UVRect tileUV(int tx, int ty) {
    float u0 = tx * TILE;
    float v1 = 1.0f - ty * TILE;
    return { u0, v1 - TILE, u0 + TILE, v1 };
}

inline BlockInfo& blockInfo(BlockType t) {
    static BlockInfo infos[] = {
        //                                                    left    right     top    bottom     back    front
        { false, false, 0.0f,  {}},//air
        { true,  true, 1.0f, {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}} },//stone
        { true,  true, 1.0f, {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}} },//dirt
        { true,  true, 1.0f, {{3,0},{3,0},{0,0},{2,0},{3,0},{3,0}} },//grass
        { true,  true, 1.0f, {{2,1},{2,1},{2,1},{2,1},{2,1},{2,1}} },//sand
        { false,  false,0.95f,{{15,13},{15,13},{15,13},{15,13},{15,13},{15,13}} },//water
    };
    return infos[t];
}

inline uint64_t chunkKey(int x, int z) {
    return ((uint64_t)(uint32_t)x << 32) | (uint32_t)z;
}

inline void addFace(std::vector<float>& verts, std::vector<unsigned int>& idx, int face, glm::vec3 base, int tx, int ty, glm::vec3 tint = {1,1,1}, float alpha = 1.0f) {
    UVRect uv = tileUV(tx, ty);

    float u[4] = {uv.u0, uv.u1, uv.u1, uv.u0};
    float v[4] = {uv.v0, uv.v0, uv.v1, uv.v1};

    unsigned int start = verts.size()/9;
    for (int i=0; i<4; i++) {
        verts.push_back(base.x+FACE[face][i][0]);
        verts.push_back(base.y+FACE[face][i][1]);
        verts.push_back(base.z+FACE[face][i][2]);
        verts.push_back(u[i]);
        verts.push_back(v[i]);
        verts.push_back(tint.r);
        verts.push_back(tint.g);
        verts.push_back(tint.b);
        verts.push_back(alpha);
    }
    idx.insert(idx.end(), { start, start+1, start+2, start+2, start+3, start });
}

class terrain {
public:
    int seed = 0;
    int renderDistance=8;
    void updateAround(engine* e, const glm::vec3& camPos, int maxLoads);
    void unloadChunk(engine* e, glm::ivec2 pos);

    BlockType getBlock(int wx, int y, int wz);
    void buildChunk(engine* e, glm::ivec2 pos);
    void meshChunk(engine* e, Chunk& c);
    void remeshDirty(engine* e, size_t maxPerCall = 4);
    bool isBlockSolid(int x, int y, int z);
    Chunk* getChunk(int cx, int cy);
    void generateChunk(Chunk& c);
    TerrainParams params;

    std::vector<Chunk*> dirty_chunks;
private:
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;

    std::vector<glm::ivec2> loadOffsets;//stored nearest -> closeset
    int cachedRadius = -1;
    glm::ivec2 lastCamChunk = { INT_MIN, INT_MIN };
    void rebuildOffsets();
};


#endif