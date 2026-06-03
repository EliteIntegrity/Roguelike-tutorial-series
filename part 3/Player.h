#pragma once
#include "Map.h"

// The player. A plain struct for now — no base class, no virtual functions.
// The Entity hierarchy (shared by Player and Monster) arrives in Part 4, when
// monsters need the same movement and combat logic. Until then a struct is
// all we need and is the easiest thing to read.
struct Player
{
    int x     = 0;
    int y     = 0;
    int hp    = 30;
    int maxHp = 30;

    // Attempt to step by (dx, dy). Returns true if the move happened — the
    // caller treats that as "a turn was taken". Walking into a wall or off the
    // edge returns false and the player stays put.
    bool tryMove(int dx, int dy, const Map& map)
    {
        int nx = x + dx;
        int ny = y + dy;

        if (nx < 0 || nx >= MAP_WIDTH)  return false;
        if (ny < 0 || ny >= MAP_HEIGHT) return false;          // bounds first...
        if (map.tiles[ny][nx].type != TileType::Floor) return false;  // ...then the tile

        x = nx;
        y = ny;
        return true;
    }
};
