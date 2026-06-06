#pragma once
#include <SDL3/SDL.h>
#include "Map.h"

// ---------------------------------------------------------------------------
//  Entity — the shared base for everything that lives on the map and acts in
//  turns. The player and every monster are Entities, so they share one set of
//  fields (position, health, how they're drawn) and one movement routine.
// ---------------------------------------------------------------------------
struct Entity
{
    int       x     = 0;
    int       y     = 0;
    int       hp    = 1;
    int       maxHp = 1;
    char      glyph = '?';
    SDL_Color color = { 255, 255, 255, 255 };
    bool      alive = true;

    bool isAlive() const { return alive && hp > 0; }

    // Step by (dx, dy) if the destination is an in-bounds floor tile. Returns
    // true if the move happened. This only knows about the MAP — being blocked
    // by other entities (and bumping them in combat) arrives in Part 5.
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
