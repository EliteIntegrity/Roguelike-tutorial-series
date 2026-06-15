#pragma once
#include <SDL3/SDL.h>
#include "Map.h"

// Timed conditions an entity can be under. Each holds a number of turns
// remaining, ticked down once per turn by the game.
enum class Status { Poisoned, Regen, Count };

struct Entity
{
    int         x       = 0;
    int         y       = 0;
    int         hp      = 1;
    int         maxHp   = 1;
    int         power   = 1;
    int         defense = 0;
    char        glyph   = '?';
    SDL_Color   color   = { 255, 255, 255, 255 };
    const char* name    = "thing";
    bool        alive   = true;

    int statusTurns[(int)Status::Count] = {};

    bool isAlive()             const { return alive && hp > 0; }
    bool hasStatus(Status s)   const { return statusTurns[(int)s] > 0; }

    // Apply a status for `turns`; re-applying refreshes to the longer duration.
    void addStatus(Status s, int turns)
    {
        if (turns > statusTurns[(int)s]) statusTurns[(int)s] = turns;
    }

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
