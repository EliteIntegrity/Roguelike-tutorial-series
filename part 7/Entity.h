#pragma once
#include <SDL3/SDL.h>
#include "Map.h"

// ---------------------------------------------------------------------------
//  Entity — shared base for the player and every monster. Part 5 adds the two
//  fields combat needs: `power` (how hard it hits) and `name` (for the log).
// ---------------------------------------------------------------------------
struct Entity
{
    int         x     = 0;
    int         y     = 0;
    int         hp    = 1;
    int         maxHp = 1;
    int         power = 1;          // damage dealt per hit
    char        glyph = '?';
    SDL_Color   color = { 255, 255, 255, 255 };
    const char* name  = "thing";
    bool        alive = true;

    bool isAlive() const { return alive && hp > 0; }

    // Map-only movement (bounds + floor). Being blocked by another entity, and
    // bumping it to attack, is handled by the game loop, which is the only
    // place that knows about every entity at once.
    bool tryMove(int dx, int dy, const Map& map)
    {
        int nx = x + dx;
        int ny = y + dy;

        if (nx < 0 || nx >= MAP_WIDTH)  return false;
        if (ny < 0 || ny >= MAP_HEIGHT) return false;
        if (map.tiles[ny][nx].type != TileType::Floor) return false;

        x = nx;
        y = ny;
        return true;
    }
};
