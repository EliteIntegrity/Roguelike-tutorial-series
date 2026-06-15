#pragma once
#include <random>
#include "Entity.h"

struct Monster : Entity
{
    bool venomous = false;   // does its bite inflict Poisoned?

    void wander(const Map& map, std::mt19937& rng)
    {
        static const int dx[4] = {  0,  0, -1, +1 };
        static const int dy[4] = { -1, +1,  0,  0 };

        int choice = std::uniform_int_distribution<int>(0, 4)(rng);
        if (choice < 4)
            tryMove(dx[choice], dy[choice], map);
    }
};
