#include "worldgen.h"
#include "terrain.h"
#include "glm/glm.hpp"

//comment

static float fbm2D(float wx, float wz, const TerrainParams& p) {
    float sum = 0.0f, amp = 1.0f, freq = p.frequency, norm = 0.0f;
    for (int i = 0; i < p.octaves; i++) {
        sum  += amp * stb_perlin_noise3_seed(wx * freq, 0.0f, wz * freq, 0, 0, 0, p.seed + i);
        norm += amp;
        amp  *= p.gain;
        freq *= p.lacunarity;
    }
    return sum / norm;
}

int heightAt(int wx, int wz, const TerrainParams& p) {
    float n = fbm2D((float)wx, (float)wz, p);
    int h = (int)(p.baseHeight + n * p.amplitude);
    return glm::clamp(h, 1, CY - 2);
}

BlockType blockFor(int x, int y, int z, int surfaceH, const TerrainParams& p) {
    if (y==0) return BEDROCK;

    //large caves
    const float freq = 0.01f;
    const float carveT = -0.5f;
    const float wallBand = 0.02f;

    float large = stb_perlin_noise3((float)x*freq, (float)y*freq, (float)z*freq, 0, 0, 0);
    if (large < carveT) return AIR;

    if (y > surfaceH) {
        if (y <= p.seaLevel) {
            if (large < carveT + wallBand) return SAND;
            return WATER;
        }
        return AIR;
    }

    //basic generation
    if (y == surfaceH) return (surfaceH <= p.seaLevel) ? SAND : GRASS;
    if (y >= surfaceH - p.dirtDepth) return DIRT;
    return STONE;
}

void terrain::generateChunk(Chunk& c) const {
    glm::ivec2 pos = c.coord;
    for (int x = 0; x < CX; x++) {
        for (int z = 0; z < CZ; z++) {
            int h = heightAt(x + pos.x * CX, z + pos.y * CZ, params);   // one noise eval per column

            for (int y = 0; y < CY; y++) {
                c.blocks[x + z*CX + y*CX*CZ] = (uint8_t)blockFor(x+pos.x*CX, y, z+pos.y*CZ, h, params);
            }
        }
    }
}

