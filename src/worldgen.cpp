#include "worldgen.h"
#include "terrain.h"
#include "glm/glm.hpp"

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

BlockType blockFor(int y, int surfaceH, const TerrainParams& p) {
    if (y == 0) return BEDROCK;
    if (y > surfaceH) return (y <= p.seaLevel) ? WATER : AIR;
    if (y == surfaceH) return (surfaceH <= p.seaLevel + 2) ? SAND : GRASS;
    if (y >= surfaceH - p.dirtDepth) return DIRT;
    return STONE;
}

void terrain::generateChunk(Chunk& c) const {
    glm::ivec2 pos = c.coord;
    for (int x = 0; x < CX; x++)
        for (int z = 0; z < CZ; z++) {
            int h = heightAt(x + pos.x * CX, z + pos.y * CZ, params);
            for (int y = 0; y < CY; y++)
                c.blocks[x + z * CX + y * CX * CZ] = (uint8_t)blockFor(y, h, params);
        }
}