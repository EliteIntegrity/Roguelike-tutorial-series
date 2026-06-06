#include "FOV.h"

// Eight sets of { xx, xy, yx, yy } multipliers. They rotate the one canonical
// octant the algorithm works in into each of the eight 45 degree sectors.
// A map coordinate is reconstructed as:  x = cx + dx*xx + dy*xy
//                                        y = cy + dx*yx + dy*yy
// where dy = -distance (it runs "up" the octant) and dx = -distance..0.
const int FOV::MULT[8][4] = {
    {  1,  0,  0,  1 },
    {  0,  1,  1,  0 },
    {  0, -1,  1,  0 },
    { -1,  0,  0,  1 },
    { -1,  0,  0, -1 },
    {  0, -1, -1,  0 },
    {  0,  1, -1,  0 },
    {  1,  0,  0, -1 },
};

void FOV::compute(Map& map, int originX, int originY)
{
    // Every tile starts the turn out of sight; the cast below re-lights what
    // can be seen. `explored` is never cleared — it is permanent memory.
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            map.tiles[y][x].visible = false;

    map.tiles[originY][originX].visible  = true;
    map.tiles[originY][originX].explored = true;

    for (int oct = 0; oct < 8; ++oct)
        castLight(map, originX, originY,
                  1, 1.0f, 0.0f,
                  MULT[oct][0], MULT[oct][1], MULT[oct][2], MULT[oct][3]);
}

// Scan one row of the octant at a time, tracking the arc of slopes that is
// still lit. A wall casts a shadow; the function recurses to handle the lit
// arc that precedes the wall, then narrows the arc for the rest of the row.
void FOV::castLight(Map& map, int cx, int cy,
                    int row, float startSlope, float endSlope,
                    int xx, int xy, int yx, int yy)
{
    if (startSlope < endSlope) return;

    float nextStartSlope = startSlope;
    bool  blocked        = false;

    for (int distance = row; distance <= RADIUS && !blocked; ++distance)
    {
        int dy = -distance;

        for (int dx = -distance; dx <= 0; ++dx)
        {
            // Slopes of this cell's left and right edges. dy is negative, so
            // the denominators are (dy + 0.5) and (dy - 0.5).
            float lSlope = (dx - 0.5f) / (dy + 0.5f);
            float rSlope = (dx + 0.5f) / (dy - 0.5f);

            if (startSlope < rSlope) continue;   // not in the lit arc yet
            if (endSlope   > lSlope) break;       // past the far edge of the arc

            int x = cx + dx * xx + dy * xy;
            int y = cy + dx * yx + dy * yy;

            if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
                continue;

            // Circular clamp — without it the lit area would be a square.
            if (dx * dx + dy * dy <= RADIUS * RADIUS)
            {
                map.tiles[y][x].visible  = true;
                map.tiles[y][x].explored = true;
            }

            bool isWall = (map.tiles[y][x].type == TileType::Wall);

            if (blocked)
            {
                if (isWall)
                {
                    nextStartSlope = rSlope;       // still in shadow
                }
                else
                {
                    blocked    = false;            // shadow ended
                    startSlope = nextStartSlope;
                }
            }
            else if (isWall && distance < RADIUS)
            {
                // Entering shadow: recurse on the lit arc above this wall,
                // then keep scanning with a tightened start slope.
                blocked = true;
                castLight(map, cx, cy, distance + 1, startSlope, lSlope, xx, xy, yx, yy);
                nextStartSlope = rSlope;
            }
        }
    }
}
