#include "terrain.h"

#include <algorithm>

#include "glm/glm.hpp"

#include "engine.h"
#include "stb_perlin.h"
#include "worldgen.h"

int floor_div(int a, int b) {
    int res = a / b;
    int rem = a % b;

    if ((a ^ b) < 0 && rem != 0) {
        res--;
    }
    return res;
}

BlockType terrain::getBlock(int wx, int y, int wz) {
    if (y < 0 || y >= CY) return AIR;
    int cx = floor_div(wx, CX), cz = floor_div(wz, CZ);
    Chunk* c = getChunk(cx, cz);
    if (!c) return AIR;
    return (BlockType)c->get(wx - cx*CX, y, wz - cz*CZ);
}

void terrain::meshChunk(engine* e, Chunk& c) {
    std::vector<float> verts, wVerts;               // opaque / water
    std::vector<unsigned int> idx, wIdx;
    glm::ivec2 pos = c.coord;

    if (c.renderId != -1)      { e->removeOpaqueRenderable(c.renderId);  c.renderId = -1; }
    if (c.waterRenderId != -1) { e->removeTRenderable(c.waterRenderId);  c.waterRenderId = -1; }

    for (int x=0; x < CX; x++)
    for (int y=0; y < CY; y++)
    for (int z=0; z < CZ; z++) {
        BlockType t = (BlockType)c.get(x,y,z);
        if (t == AIR) continue;
        const BlockInfo& info = blockInfo(t);

        for (int f = 0; f < 6; f++) {
            glm::ivec3 n = glm::ivec3(x,y,z) + DIR[f];

            BlockType nt = Chunk::inBounds(n.x,n.y,n.z)
                ? (BlockType)c.get(n.x,n.y,n.z)
                : getBlock(n.x + CX*pos.x, n.y, n.z + CZ*pos.y);

            // hidden behind an opaque block, or inside a body of the same transparent block
            if (nt != AIR && (blockInfo(nt).opaque || nt == t)) continue;

            glm::vec3 tint = {1,1,1};
            if (t == GRASS && f == 2) tint = {0.65f, 0.95f, 0.50f};
            if (t == WATER)           tint = {0.45f, 0.65f, 1.0f};   // remove if your tile is already blue

            glm::vec3 base = {(float)(x+CX*pos.x), (float)y, (float)(z+CZ*pos.y)};
            auto& V = info.opaque ? verts : wVerts;
            auto& I = info.opaque ? idx   : wIdx;
            addFace(V, I, f, base, info.tex[f][0], info.tex[f][1], tint, info.alpha);
        }
    }

    if (!verts.empty()) {
        object o(e, false, verts, idx);
        c.renderId = e->submitOpaqueRenderable(o);
    }
    if (!wVerts.empty()) {
        object o(e, false, wVerts, wIdx);
        o.sortPos = { pos.x*CX + CX*0.5f, (float)params.seaLevel, pos.y*CZ + CZ*0.5f };
        c.waterRenderId = e->submitTRenderable(o);
    }
    c.dirty = false;
}

//generates chunk in the given position
void terrain::buildChunk(engine* e, glm::ivec2 pos) {
    // replace an existing chunk
    auto it = chunks.find(chunkKey(pos.x, pos.y));
    if (it != chunks.end()) {
        Chunk* old = it->second.get();
        if (old->renderId != -1)      e->removeOpaqueRenderable(old->renderId);
        if (old->waterRenderId != -1) e->removeTRenderable(old->waterRenderId);
        std::erase(dirty_chunks, old);
        chunks.erase(it);
    }

    auto c = std::make_unique<Chunk>(pos);
    generateChunk(*c);

    Chunk* raw = c.get();
    chunks[chunkKey(pos.x, pos.y)] = std::move(c);
    meshChunk(e, *raw);

    auto markDirty = [&](int cx, int cz) {
        Chunk* n = getChunk(cx, cz);
        if (n && !n->dirty) { n->dirty = true; dirty_chunks.push_back(n); }
    };
    markDirty(pos.x-1, pos.y);
    markDirty(pos.x+1, pos.y);
    markDirty(pos.x, pos.y-1);
    markDirty(pos.x, pos.y+1);
}

//updates chunks waiting to be updated
void terrain::remeshDirty(engine* e, size_t maxPerCall) {
    size_t done = 0;
    while (!dirty_chunks.empty() && done < maxPerCall) {
        Chunk* c = dirty_chunks.back();
        dirty_chunks.pop_back();
        if (!c->dirty) continue;
        meshChunk(e, *c); //clears dirty queue
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
    if (cachedRadius != renderDistance) rebuildOffsets();

    glm::ivec2 cc = { (int)std::floor(camPos.x / CX), (int)std::floor(camPos.z / CZ) };

    if (cc != lastCamChunk) {
        lastCamChunk = cc;
        int unloadR = renderDistance + 2;
        std::vector<glm::ivec2> far;
        for (auto& [key, c] : chunks) {
            glm::ivec2 d = c->coord - cc;
            if (d.x*d.x + d.y*d.y > unloadR*unloadR) far.push_back(c->coord);
        }
        for (auto& p : far) unloadChunk(e, p);
    }

    // load missing chunks, nearest first
    int loaded = 0;
    for (const glm::ivec2& off : loadOffsets) {
        glm::ivec2 p = cc + off;
        if (getChunk(p.x, p.y)) continue;
        buildChunk(e, p);
        if (++loaded >= maxLoads) break;
    }
}