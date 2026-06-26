#include "angleHelper.hpp"

float normalizeDeg(float deg)
{
    float norm = fmodf(deg + 360.0f, 360);
    if (norm > 180)
        norm -= 360;
    return norm;
}

float diffDeg(float deg1, float deg2)
{
    deg1 = normalizeDeg(deg1);
    deg2 = normalizeDeg(deg2);
    float diff = (deg1 - deg2);

    float mod = fmodf(diff + 360.0f, 360);
    if (mod > 180)
        mod -= 360;
    return mod;
}

float nearSessenDeg(float target_deg, float enter_deg)
{
    float sessen1_deg = normalizeDeg(target_deg + 90);
    float sessen2_deg = normalizeDeg(target_deg - 90);

    bool is_sessen1_deg_near = abs(diffDeg(sessen1_deg, enter_deg)) < abs(diffDeg(sessen2_deg, enter_deg));

    return is_sessen1_deg_near ? sessen1_deg : sessen2_deg;
}

float areaIndexFromDeg(int n, float deg)
{
    float area_size = 360.0f / n;

    float positiveDeg = fmodf(deg + 360, 360);

    int index = (int)fmodf((positiveDeg + area_size / 2.0f) / area_size, n);
    return index;
}

float degFromAreaIndex(int n, int index)
{
    float area_size = 360.0f / n;

    float positiveDeg = fmodf(index * area_size + area_size / 2.0f, 360);

    return normalizeDeg(positiveDeg);
}

Area4 area4(int deg)
{
    return Area4(areaIndexFromDeg(4, deg));
}

Area8 area8(int deg)
{
    return Area8(areaIndexFromDeg(8, deg));
}

Area16 area16(int deg)
{
    return Area16(areaIndexFromDeg(16, deg));
}