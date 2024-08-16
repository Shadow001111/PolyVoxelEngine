#include <cmath>
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>

class SimplexNoise
{
public:
    SimplexNoise(unsigned int seed);
    float noise(float xin, float yin) const;
    float noise(float xin, float yin, float zin) const;

    unsigned char getPerm(int index) const;
private:
    int perm[256];
};
