#pragma once
#include "Map.h"

// ---------------------------------------------------------------------------
//  Field of view by recursive shadowcasting — the algorithm shipped by
//  NetHack, Angband, DCSS and most serious roguelikes. It works in one 45°
//  octant and is run eight times (once per octant) to cover all directions.
// ---------------------------------------------------------------------------
class FOV
{
public:
    static constexpr int RADIUS = 11;   // how far the player can see, in tiles

    // Clear all visibility, then light every tile in line of sight of
    // (originX, originY) and mark it explored.
    static void compute(Map& map, int originX, int originY);

private:
    static const int MULT[8][4];        // per-octant coordinate transforms

    static void castLight(Map& map, int cx, int cy,
                          int row, float startSlope, float endSlope,
                          int xx, int xy, int yx, int yy);
};
