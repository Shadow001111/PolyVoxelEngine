#include "SimplexNoise.h"
#include <numeric>
#include <random>

static constexpr inline int fastfloor(float x)
{
    return x > 0 ? (int)x : (int)x - 1;
}

static constexpr inline float dot(const int* g, float x, float y)
{
    return g[0] * x + g[1] * y;
}

static constexpr inline float dot(const int* g, float x, float y, float z)
{
    return g[0] * x + g[1] * y + g[2] * z;
}

constexpr int grad2[16] = 
{
    1, 1, -1, 1, 1, -1, -1, -1, 1, 0, -1, 0, 0, 1, 0, -1
};

constexpr int grad3[36] =
{
    1, 1, 0, -1, 1, 0, 1, -1, 0, -1, -1, 0,
    1, 0, 1, -1, 0, 1, 1, 0, -1, -1, 0, -1,
    0, 1, 1, 0, -1, 1, 0, 1, -1, 0, -1, -1
};

SimplexNoise::SimplexNoise(unsigned int seed)
{
    std::iota(perm, perm + 256, 0);
    std::default_random_engine engine(seed);
    std::shuffle(perm, perm + 256, engine);
}

float SimplexNoise::noise(float xin, float yin) const
{
    constexpr float F2 = 0.366025f;
    constexpr float G2 = 0.211325f;
    constexpr float K0 = 0.5f;
    constexpr float K1 = 2.0f * G2 - 1.0f;

    float s = (xin + yin) * F2;
    int i = fastfloor(xin + s);
    int j = fastfloor(yin + s);
    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = xin - X0;
    float y0 = yin - Y0;

    int i1, j1;
    if (x0 > y0) 
    {
        i1 = 1; j1 = 0; 
    }
    else 
    {
        i1 = 0; j1 = 1; 
    }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 + K1;
    float y2 = y0 + K1;

    unsigned char gi0 = getPerm(i + getPerm(j)) & 7;
    unsigned char gi1 = getPerm(i + i1 + getPerm(j + j1)) & 7;
    unsigned char gi2 = getPerm(i + 1 + getPerm(j + 1)) & 7;

    float t0 = K0 - x0 * x0 - y0 * y0;
    float n0 = 0.0f;
    if (t0 >= 0.0f)
    {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad2 + (gi0 << 1), x0, y0);
    }

    float t1 = K0 - x1 * x1 - y1 * y1;
    float n1 = 0.0f;
    if (t1 >= 0.0f)
    {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad2 + (gi1 << 1), x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    float n2 = 0.0f;
    if (t2 >= 0.0f)
    {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad2 + (gi2 << 1), x2, y2);
    }

    return 70.0f * (n0 + n1 + n2);
}

float SimplexNoise::noise(float xin, float yin, float zin) const
{
    constexpr float F3 = 1.0f / 3.0f;
    constexpr float G3 = 1.0f / 6.0f;
    constexpr float K0 = 0.6f;
    constexpr float K1 = 2.0f * G3;
    constexpr float K2 = 3.0f * G3 - 1.0f;

    float s = (xin + yin + zin) * F3;
    int i = fastfloor(xin + s);
    int j = fastfloor(yin + s);
    int k = fastfloor(zin + s);
    float t = (i + j + k) * G3;
    float x0 = xin - i + t;
    float y0 = yin - j + t;
    float z0 = zin - k + t;

    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) 
    {
        if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
        else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
    }
    else 
    {
        if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
        else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
        else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
    }

    float x1 = x0 - i1 + G3;
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + K1;
    float y2 = y0 - j2 + K1;
    float z2 = z0 - k2 + K1;
    float x3 = x0 + K2;
    float y3 = y0 + K2;
    float z3 = z0 + K2;

    int gi0 = getPerm(i + getPerm(j + getPerm(k))) % 12;
    int gi1 = getPerm(i + i1 + getPerm(j + j1 + getPerm(k + k1))) % 12;
    int gi2 = getPerm(i + i2 + getPerm(j + j2 + getPerm(k + k2))) % 12;
    int gi3 = getPerm(i + 1 + getPerm(j + 1 + getPerm(k + 1))) % 12;

    float t0 = K0 - x0 * x0 - y0 * y0 - z0 * z0;
    float n0 = 0.0f;
    if (t0 >= 0.0f)
    {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3 + gi0 * 3, x0, y0, z0);
    }

    float t1 = K0 - x1 * x1 - y1 * y1 - z1 * z1;
    float n1 = 0.0f;
    if (t1 >= 0.0f)
    {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3 + gi1 * 3, x1, y1, z1);
    }

    float t2 = K0 - x2 * x2 - y2 * y2 - z2 * z2;
    float n2 = 0.0f;
    if (t2 >= 0.0f)
    {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3 + gi2 * 3, x2, y2, z2);
    }

    float t3 = K0 - x3 * x3 - y3 * y3 - z3 * z3;
    float n3 = 0.0f;
    if (t3 >= 0.0f)
    {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad3 + gi3 * 3, x3, y3, z3);
    }

    return 32.0f * (n0 + n1 + n2 + n3);
}

unsigned char SimplexNoise::getPerm(unsigned char index) const
{
    return perm[index];
}