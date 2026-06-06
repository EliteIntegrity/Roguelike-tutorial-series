#pragma once
#include <SDL3/SDL.h>
#include "Map.h"

// Part 9 adds `defense`. Damage taken is reduced by the defender's defense, so
// armour finally matters. (Monsters leave it at 0 unless a kind sets it.)
struct Entity
{
    int         x       = 0;
    int         y       = 0;
    int         hp      = 1;
    int         maxHp   = 1;
    int         power   = 1;     // attack
    int         defense = 0;     // subtracted from incoming damage
    char        glyph   = '?';
    SDL_Color   color   = { 255, 255, 255, 255 };
    const char* name    = "thing";
    bool        alive   = true;

    bool isAlive() const { return alive && hp > 0; }

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
