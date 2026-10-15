#pragma once
#include <cstdint>
#include <cmath>
#include "terrain.h"

class JRandom {
    uint64_t s = 0;
    static constexpr uint64_t MASK = (1ULL << 48) - 1;
    int next(int bits) {
        s = (s * 0x5DEECE66DULL + 0xBULL) & MASK;
        return (int32_t)(uint32_t)(s >> (48 - bits));
    }
public:
    explicit JRandom(int64_t seed = 0) { setSeed(seed); }
    void setSeed(int64_t seed) { s = ((uint64_t)seed ^ 0x5DEECE66DULL) & MASK; }

    int nextInt(int bound) {
        if ((bound & -bound) == bound) return (int)(((int64_t)bound * next(31)) >> 31);
        int bits, val;
        do {
            bits = next(31);
            val = bits % bound;
        } while ((int32_t)((uint32_t)bits - (uint32_t)val + (uint32_t)(bound - 1)) < 0);
        return val;
    }
    int64_t nextLong() {
        uint64_t hi = (uint64_t)(int64_t)next(32);
        uint64_t lo = (uint64_t)(int64_t)next(32);
        return (int64_t)((hi << 32) + lo);
    }
    float nextFloat() { return next(24) / (float)(1 << 24); }
};

class CaveCarver {
    static constexpr int RANGE = 8;
    static constexpr double PI = 3.14159265358979323846;
    int64_t worldSeed;

    static int64_t mul(int a, int64_t b) { return (int64_t)((uint64_t)(int64_t)a * (uint64_t)b); }

public:
    explicit CaveCarver(int64_t seed) : worldSeed(seed) {}

    void carve(Chunk& c) const {
        JRandom rand(worldSeed);
        int64_t a = rand.nextLong();
        int64_t b = rand.nextLong();
        for (int cx = c.coord.x - RANGE; cx <= c.coord.x + RANGE; cx++)
            for (int cz = c.coord.y - RANGE; cz <= c.coord.y + RANGE; cz++) {
                rand.setSeed(mul(cx, a) ^ mul(cz, b) ^ worldSeed);
                startCaves(rand, cx, cz, c);
            }
    }

private:
    void startCaves(JRandom& rand, int ocx, int ocz, Chunk& target) const {
        int n = rand.nextInt(rand.nextInt(rand.nextInt(40) + 1) + 1);
        if (rand.nextInt(15) != 0) n = 0;

        for (int i = 0; i < n; i++) {
            double x = ocx * 16 + rand.nextInt(16);
            double y = rand.nextInt(rand.nextInt(120) + 8);
            double z = ocz * 16 + rand.nextInt(16);

            int tunnels = 1;
            if (rand.nextInt(4) == 0) {  //big round room
                int64_t seed = rand.nextLong();
                float size = 1.0f + rand.nextFloat() * 6.0f;
                tunnel(seed, target, x, y, z, size, 0, 0, -1, -1, 0.5);
                tunnels += rand.nextInt(4);
            }
            for (int t = 0; t < tunnels; t++) {
                float yaw   = rand.nextFloat() * (float)PI * 2.0f;
                float pitch = (rand.nextFloat() - 0.5f) * 2.0f / 8.0f;
                float s1 = rand.nextFloat();
                float s2 = rand.nextFloat();
                float size = s1 * 2.0f + s2;
                int64_t seed = rand.nextLong();
                tunnel(seed, target, x, y, z, size, yaw, pitch, 0, 0, 1.0);
            }
        }
    }

    void tunnel(int64_t nodeSeed, Chunk& c, double x, double y, double z,
                float size, float yaw, float pitch, int step, int maxStep, double yScale) const {
        const double midX = c.coord.x * 16 + 8.0, midZ = c.coord.y * 16 + 8.0;
        float yawVel = 0, pitchVel = 0;

        JRandom rand(nodeSeed);
        if (maxStep <= 0) {
            int j = RANGE * 16 - 16;
            maxStep = j - rand.nextInt(j / 4);
        }
        bool room = false;
        if (step == -1) { step = maxStep / 2; room = true; }

        int splitAt = rand.nextInt(maxStep / 2) + maxStep / 4;
        bool steep = rand.nextInt(6) == 0;

        for (; step < maxStep; step++) {
            double radius  = 1.5 + std::sin(step * PI / maxStep) * size;
            double radiusY = radius * yScale;

            float cp = std::cos(pitch), sp = std::sin(pitch);
            x += std::cos(yaw) * cp;
            y += sp;
            z += std::sin(yaw) * cp;

            pitch *= steep ? 0.92f : 0.7f;
            pitch += pitchVel * 0.1f;
            yaw   += yawVel * 0.1f;
            pitchVel *= 0.9f;
            yawVel   *= 0.75f;

            float r1 = rand.nextFloat(), r2 = rand.nextFloat(), r3 = rand.nextFloat();
            pitchVel += (r1 - r2) * r3 * 2.0f;
            float r4 = rand.nextFloat(), r5 = rand.nextFloat(), r6 = rand.nextFloat();
            yawVel   += (r4 - r5) * r6 * 4.0f;

            if (!room && step == splitAt && size > 1.0f && maxStep > 0) {
                int64_t sa = rand.nextLong();
                float za = rand.nextFloat() * 0.5f + 0.5f;
                tunnel(sa, c, x, y, z, za, yaw - (float)PI / 2, pitch / 3, step, maxStep, 1.0);
                int64_t sb = rand.nextLong();
                float zb = rand.nextFloat() * 0.5f + 0.5f;
                tunnel(sb, c, x, y, z, zb, yaw + (float)PI / 2, pitch / 3, step, maxStep, 1.0);
                return;
            }
            if (!room && rand.nextInt(4) == 0) continue;

            //too far from tgt chunk
            double dx = x - midX, dz = z - midZ, left = maxStep - step, lim = size + 2 + 16;
            if (dx * dx + dz * dz - left * left > lim * lim) return;
            if (x < midX - 16 - radius * 2 || x > midX + 16 + radius * 2 ||
                z < midZ - 16 - radius * 2 || z > midZ + 16 + radius * 2) continue;

            carveSphere(c, x, y, z, radius, radiusY);
        }
    }

    void carveSphere(Chunk& c, double x, double y, double z, double radius, double radiusY) const {
        const int baseX = c.coord.x * 16, baseZ = c.coord.y * 16;
        int minX = (int)std::floor(x - radius) - baseX - 1, maxX = (int)std::floor(x + radius) - baseX + 1;
        int minY = (int)std::floor(y - radiusY) - 1,        maxY = (int)std::floor(y + radiusY) + 1;
        int minZ = (int)std::floor(z - radius) - baseZ - 1, maxZ = (int)std::floor(z + radius) - baseZ + 1;
        if (minX < 0) minX = 0;
        if (maxX > CX) maxX = CX;
        if (minZ < 0) minZ = 0;
        if (maxZ > CZ) maxZ = CZ;
        if (minY < 1) minY = 1;
        if (maxY > CY - 8) maxY = CY - 8;
        if (minX >= maxX || minY >= maxY || minZ >= maxZ) return;

        // avoid lakes/oceans
        for (int xi = minX; xi < maxX; xi++)
            for (int zi = minZ; zi < maxZ; zi++)
                for (int yi = std::min(maxY + 1, CY - 1); yi >= minY - 1; yi--)
                    if (c.get(xi, yi, zi) == WATER) return;

        for (int xi = minX; xi < maxX; xi++) {
            double dx = ((xi + baseX) + 0.5 - x) / radius;
            for (int zi = minZ; zi < maxZ; zi++) {
                double dz = ((zi + baseZ) + 0.5 - z) / radius;
                if (dx * dx + dz * dz >= 1.0) continue;

                bool hitGrass = false;
                for (int yi = maxY - 1; yi >= minY; yi--) {
                    double dy = (yi + 0.5 - y) / radiusY;
                    if (dy > -0.7 && dx * dx + dy * dy + dz * dz < 1.0) {
                        uint8_t& b = c.blocks[xi + zi * CX + yi * CX * CZ];
                        if (b == GRASS) hitGrass = true;
                        if (b == STONE || b == DIRT || b == GRASS) {
                            b = AIR; //lava here
                            if (hitGrass) {
                                uint8_t& below = c.blocks[xi + zi * CX + (yi - 1) * CX * CZ];
                                if (below == DIRT) below = GRASS;
                            }
                        }
                    }
                }
            }
        }
    }
};