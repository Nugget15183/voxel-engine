#ifndef VOXEL_ENGINE_TERRAIN_H
#define VOXEL_ENGINE_TERRAIN_H

#include <bit>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_set>
#include <glm/glm.hpp>

#include "logger.h"
#include "worldgen.h"

//comment

class engine;

//const+constexprs
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
    BEDROCK,
};

//structs
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

    bool meshed = false;
    bool dirty = false;
    int renderId = -1;
    int waterRenderId = -1;

    static bool inBounds(int x, int y, int z) {
        return x >= 0 && x < CX && y >= 0 && y < CY && z >= 0 && z < CZ;
    }

    uint8_t get(int x, int y, int z) const {
        if (!inBounds(x, y, z)) return AIR;
        return blocks[x + z*CX + y*CX*CZ];
    }
};
struct ChunkCoord {int x,y;};
struct Meshdata {std::vector<float> opaqueVerts, transparentVerts; std::vector<unsigned int> opaqueIdx, transparentIdx;};
struct ChunkResult {ChunkCoord coord;  std::unique_ptr<Chunk> chunk; Meshdata mesh;};

//voids
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
        { false,  false,0.9f,{{15,13},{15,13},{15,13},{15,13},{15,13},{15,13}} },//water
        { true,  true, 1.0f, {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1}} },//bedrock
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

    unsigned int start = verts.size()/10;
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
        verts.push_back((float)face);
    }
    idx.insert(idx.end(), { start, start+1, start+2, start+2, start+3, start });
}
int       heightAt(int wx, int wz, const TerrainParams& p);
BlockType blockFor(int x, int y, int z, int surfaceH, const TerrainParams& p);

//classes
template <typename T>
class SafeQueue {
public:
    //add an item to the queue
    void push(T item) {
        {
            lock_guard<mutex> lock(m);
            q.push(move(item));
        }
        cv.notify_one(); //wake a worker
    }

    optional<T> waitPop() {
        unique_lock<mutex> lock(m);

        //wait for the thread to either finish or close
        cv.wait(lock, [&]{return !q.empty() || closed;});

        //closed queue and its drained
        if (closed || q.empty()) return nullopt;

        //grab the item at the front of the queue
        T item = move(q.front());

        //take the last item off the queue and return the item
        q.pop();
        return item;
    }

    optional<T> tryPop() {
        lock_guard<mutex> lock(m);

        //if empty queue return, theres nothing to pop
        if (q.empty()) return nullopt;

        //grab the item at the front of the queue
        T item = move(q.front());

        //pop last item off the queue and return
        q.pop();
        return item;
    }

    void close() {
        {
            //close the queue
            lock_guard<mutex> lock(m);
            closed = true;
        }

        //notify all of the now closed queue
        cv.notify_all();
    }
private:
    queue<T> q;
    mutex m;
    condition_variable cv;
    bool closed = false;
};

class CWorker {
public:
    using WorkFn = std::function<ChunkResult(ChunkCoord)>;

    //starts the threads
    void start(int tcount, WorkFn fn) {
        work = std::move(fn);
        for (int i = 0; i < tcount; i++)
            threads.emplace_back([this] { run(); });
    }

    //stops the threads
    void stop() {
        requests.close();
        for (auto& t : threads) if (t.joinable()) t.join();
        threads.clear();
    }

    //stops worker on destruction
    ~CWorker() { stop(); }

    SafeQueue<ChunkCoord>  requests;
    SafeQueue<ChunkResult> results;
private:
    void run() {
        //waits for jobs to appear and starts them once they do
        while (auto job = requests.waitPop())
            results.push(work(*job));
    }

    WorkFn work;
    std::vector<std::thread> threads;
};

class terrain {
public:
    int seed = 0;
    int renderDistance=8;//gets set to the camera's renderdistance later

    BlockType getBlock(int wx, int y, int wz);

    ChunkResult buildChunkData(glm::ivec2 pos); //runs on worker threads
    Chunk* getChunk(int cx, int cy);

    void updateAround(engine* e, const glm::vec3& camPos, int maxLoads);
    void unloadChunk(engine* e, glm::ivec2 pos);
    void buildChunk(engine* e, glm::ivec2 pos);
    void meshChunk(engine* e, Chunk& c);
    void remeshDirty(engine* e, size_t maxPerCall = 4);
    void generateChunk(Chunk& c) const;
    void uploadMesh(engine* e, Chunk& c, Meshdata& m); //main thread only
    bool isBlockSolid(int x, int y, int z);

    TerrainParams params;
    std::vector<Chunk*> dirty_chunks;
private:
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;
    std::vector<glm::ivec2> loadOffsets;//stored nearest -> closeset

    int cachedRadius = -1;
    glm::ivec2 lastCamChunk = { INT_MIN, INT_MIN };
    void rebuildOffsets();
    logger l = logger();

    unordered_set<uint64_t> pending;
    bool workersStarted = false;
    CWorker worker;
};

#endif