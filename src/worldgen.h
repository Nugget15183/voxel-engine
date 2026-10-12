//
// Created by conte on 10/11/2026.
//

#ifndef VOXEL_ENGINE_WORLDGEN_H
#define VOXEL_ENGINE_WORLDGEN_H

#include "worldgen.h"
#include "stb_perlin.h"

struct TerrainParams {
    int   seed        = 1337;
    int   seaLevel    = 62;
    float baseHeight  = 64.0f;   // average ground level
    float amplitude   = 28.0f;   // max deviation from baseHeight
    float frequency   = 0.016f;  // lower = broader hills
    int   octaves     = 5;
    float lacunarity  = 2.0f;    // frequency multiplier per octave
    float gain        = 0.5f;    // amplitude multiplier per octave
    int   dirtDepth   = 3;       // dirt layers under the surface
};

class worldgen {

};


#endif //VOXEL_ENGINE_WORLDGEN_H