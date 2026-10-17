#include "terrain.h"

#include <algorithm>

#include "glm/glm.hpp"

#include "engine.h"
#include "stb_perlin.h"
#include "worldgen.h"
#include "cavecarver.h"

//comment

int floor_div(int a, int b) {
    int res = a / b;
    int rem = a % b;

    if ((a ^ b) < 0 && rem != 0) {
        res--;
    }
    return res;
}

template <class NeighborFn>
static void buildMeshData(const Chunk& c, NeighborFn neighborAt, Meshdata& out) {
    glm::ivec2 pos = c.coord;

    //create a heightmap for lighting calcs
    int heightmap[CX][CZ];
    for (int x = 0; x < CX; x++)
        for (int z = 0; z < CZ; z++) {
            int h = 0;
            for (int y = CY - 1; y >= 0; y--) {
                BlockType b = (BlockType)c.get(x, y, z);
                if (b != AIR && blockInfo(b).opaque) { h = y; break; }
            }
            heightmap[x][z] = h;
        }

    for (int x = 0; x < CX; x++)
        for (int y = 0; y < CY; y++)
            for (int z = 0; z < CZ; z++) {
                BlockType t = (BlockType)c.get(x, y, z);
                if (t == AIR) continue;
                const BlockInfo& info = blockInfo(t);

                for (int f = 0; f < 6; f++) {
                    glm::ivec3 n = glm::ivec3(x, y, z) + DIR[f];

                    BlockType nt = Chunk::inBounds(n.x, n.y, n.z)
                        ? (BlockType)c.get(n.x, n.y, n.z)
                        : neighborAt(n.x + CX*pos.x, n.y, n.z + CZ*pos.y);

                    if (nt != AIR && (blockInfo(nt).opaque || nt == t)) continue;

                    glm::vec3 tint = {1, 1, 1};
                    glm::vec3 base = {(float)(x + CX*pos.x), (float)y, (float)(z + CZ*pos.y)};
                    auto& V = info.opaque ? out.opaqueVerts : out.transparentVerts;
                    auto& I = info.opaque ? out.opaqueIdx   : out.transparentIdx;

                    if (t == GRASS && f == 2) tint = {0.65f, 0.95f, 0.50f};
                    if (t == WATER && f == 2) tint = {0.0f, 0.0f, 1.0f};

                    //temp lighting calculations based on depth
                    int depth = glm::clamp(heightmap[x][z]-y, 0, 15);
                    addFace(V, I, f, base, info.tex[f][0], info.tex[f][1], depth, tint, info.alpha);
                }
            }
}

//uploads the mesh to be rendered
void terrain::uploadMesh(engine* e, Chunk& c, Meshdata& m) {
    if (c.renderId != -1)      { e->removeOpaqueRenderable(c.renderId);  c.renderId = -1; }
    if (c.waterRenderId != -1) { e->removeTRenderable(c.waterRenderId);  c.waterRenderId = -1; }

    glm::ivec2 pos = c.coord;
    if (!m.opaqueVerts.empty()) {
        object o(e, false, std::move(m.opaqueVerts), std::move(m.opaqueIdx));
        c.renderId = e->submitOpaqueRenderable(std::move(o));
    }
    if (!m.transparentVerts.empty()) {
        object o(e, false, std::move(m.transparentVerts), std::move(m.transparentIdx));
        o.sortPos = { pos.x*CX + CX*0.5f, (float)params.seaLevel, pos.y*CZ + CZ*0.5f };
        c.waterRenderId = e->submitTRenderable(std::move(o));
    }
    c.dirty = false;
    c.meshed=true;
}

//gets the block at world coords
BlockType terrain::getBlock(int wx, int y, int wz) {
    if (y < 0 || y >= CY) return AIR;
    int cx = floor_div(wx, CX), cz = floor_div(wz, CZ);
    Chunk* c = getChunk(cx, cz);
    if (!c) return AIR;
    return (BlockType)c->get(wx - cx*CX, y, wz - cz*CZ);
}

//creates opaque and transparent vertices and indicies
void terrain::meshChunk(engine* e, Chunk& c) {
    Meshdata m;
    buildMeshData(c, [this](int x,int y,int z) {return getBlock(x, y, z);}, m);
    uploadMesh(e, c, m);
}

//builds chunk data
ChunkResult terrain::buildChunkData(glm::ivec2 pos) {
    ChunkResult r;
    r.coord = {pos.x, pos.y};
    r.chunk = std::make_unique<Chunk>(pos);
    generateChunk(*r.chunk);
    CaveCarver(params.seed).carve(*r.chunk);
    return r;
}

//updates chunks waiting to be updated
void terrain::remeshDirty(engine* e, size_t maxPerCall) {
    size_t done = 0;
    while (!dirty_chunks.empty() && done < maxPerCall) {
        Chunk* c = dirty_chunks.back();
        dirty_chunks.pop_back();
        if (!c->dirty) continue;
        if (!getChunk(c->coord.x-1, c->coord.y) || !getChunk(c->coord.x+1, c->coord.y) ||
            !getChunk(c->coord.x, c->coord.y-1) || !getChunk(c->coord.x, c->coord.y+1)) {
            c->dirty = false;   // gets remarked when the missing neighbor arrives
            continue;
            }
        meshChunk(e, *c);
        done++;
    }
}

//checks if the block at the position inputted is solid
bool terrain::isBlockSolid(int x, int y, int z) {
    int cx = floor_div(x, CX);
    int cz = floor_div(z, CZ);

    Chunk* c = getChunk(cx, cz);

    if (!c) return false;

    return c->get(x - cx*CX, y, z - cz*CZ) != AIR;
}

//returns chunk at the given coords
Chunk* terrain::getChunk(int cx, int cz) {
    auto it = chunks.find(chunkKey(cx, cz));
    return it == chunks.end() ? nullptr : it->second.get();
}

void terrain::rebuildOffsets() {
    loadOffsets.clear();
    int r = renderDistance;
    for (int dx = -r; dx <= r; dx++)
        for (int dz = -r; dz <= r; dz++)
            if (dx*dx + dz*dz <= r*r) loadOffsets.push_back({dx, dz});

    std::sort(loadOffsets.begin(), loadOffsets.end(), [](glm::ivec2 a, glm::ivec2 b) {
        return a.x*a.x + a.y*a.y < b.x*b.x + b.y*b.y;   // nearest first
    });
    cachedRadius = r;
}

void terrain::unloadChunk(engine* e, glm::ivec2 pos) {
    auto it = chunks.find(chunkKey(pos.x, pos.y));
    if (it == chunks.end()) return;

    Chunk* c = it->second.get();
    if (c->renderId != -1)      e->removeOpaqueRenderable(c->renderId);
    if (c->waterRenderId != -1) e->removeTRenderable(c->waterRenderId);
    std::erase(dirty_chunks, c);
    chunks.erase(it);
}

void terrain::updateAround(engine* e, const glm::vec3& camPos, int maxLoads) {
    if (!workersStarted) {
        //max thread count
        int hc = (int)thread::hardware_concurrency();

        worker.start(
            max(1, hc-2),//thread count, max count - 2
            [this](ChunkCoord c) {return buildChunkData({c.x, c.y});}//give the worker the function that builds chunk data
        );

        workersStarted = true;
    }

    if (cachedRadius != renderDistance) rebuildOffsets();

    glm::ivec2 cc = { (int)std::floor(camPos.x / CX), (int)std::floor(camPos.z / CZ) };
    int unloadR = renderDistance + 2;

    if (cc != lastCamChunk) {
        lastCamChunk = cc;

        std::vector<glm::ivec2> far;
        for (auto& [key, c] : chunks) {
            glm::ivec2 d = c->coord - cc;
            if (d.x*d.x + d.y*d.y > unloadR*unloadR) far.push_back(c->coord);
        }
        for (auto& p : far) unloadChunk(e, p);
    }

    int requested = 0;
    for (glm::ivec2& off : loadOffsets ) {
        //target chunk coord is camera chunk + offset
        glm::ivec2 p = cc+off;

        //grab the chunk key
        uint64_t key = chunkKey(p.x, p.y);

        //if the chunk is already loaded then ignore it
        if (chunks.count(key) || pending.count(key)) continue;

        //mark the chunk as in pending
        pending.insert(key);

        worker.requests.push({p.x, p.y});
        if (++requested >= 8) break; //requests per frame
    }

    // load missing chunks, nearest first
    int uploaded = 0;
    while (uploaded < maxLoads) {
        //grab the top chunk
        auto r = worker.results.tryPop();

        //no chunks to upload? then stop
        if (!r) break;

        //grab the key and remove it from pending, as its finished
        uint64_t key = chunkKey(r->coord.x, r->coord.y);
        pending.erase(key);

        //distance from the player to the chunk
        glm::ivec2 d = glm::ivec2(r->coord.x, r->coord.y) - cc;
        if (d.x*d.x + d.y*d.y > unloadR*unloadR) continue; //should the chunk be discarded?

        //upload and register the chunk
        Chunk* raw = r->chunk.get();
        chunks[key] = std::move(r->chunk);

        auto mark = [&](int cx, int cz) {
            Chunk* n = getChunk(cx, cz);
            if (n && !n->meshed && !n->dirty) { n->dirty = true; dirty_chunks.push_back(n); }
        };
        mark(raw->coord.x, raw->coord.y);
        mark(raw->coord.x - 1, raw->coord.y);
        mark(raw->coord.x + 1, raw->coord.y);
        mark(raw->coord.x, raw->coord.y - 1);
        mark(raw->coord.x, raw->coord.y + 1);
        uploaded++;
    }
}