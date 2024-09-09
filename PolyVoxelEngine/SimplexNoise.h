#pragma once

class SimplexNoise
{
public:
    SimplexNoise(unsigned int seed);
    float noise(float xin, float yin) const;
    float noise(float xin, float yin, float zin) const;
private:
    unsigned char perm[256];
    unsigned char getPerm(unsigned char index) const;
};
