#pragma once
#include <random>
#include "Entity.h"

// A Monster is an Entity with behaviour. When it can't see the player it
// wanders (from Part 4). When it can, the game loop drives it to chase and
// attack instead — see main.cpp.
struct Monster : Entity
{
    void wander(const Map& map, std::mt19937& rng)
    {
        static const int dx[4] = {  0,  0, -1, +1 };
        static const int dy[4] = { -1, +1,  0,  0 };

        int choice = std::uniform_int_distribution<int>(0, 4)(rng);   // 4 = wait
        if (choice < 4)
            tryMove(dx[choice], dy[choice], map);
    }
};
