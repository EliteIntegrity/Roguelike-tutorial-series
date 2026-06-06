#pragma once
#include <random>
#include "Entity.h"

// A Monster is an Entity with behaviour. In Part 4 that behaviour is simple:
// wander. Each turn it steps in a random direction (or stands still). Part 5
// gives it the sense to chase and fight the player.
struct Monster : Entity
{
    // Take one turn: a random step N/S/E/W, or wait. Movement is blocked by
    // walls (via Entity::tryMove); a blocked step simply does nothing.
    void wander(const Map& map, std::mt19937& rng)
    {
        static const int dx[4] = {  0,  0, -1, +1 };
        static const int dy[4] = { -1, +1,  0,  0 };

        // 0..3 = move in that direction, 4 = wait this turn.
        int choice = std::uniform_int_distribution<int>(0, 4)(rng);
        if (choice < 4)
            tryMove(dx[choice], dy[choice], map);
    }
};
