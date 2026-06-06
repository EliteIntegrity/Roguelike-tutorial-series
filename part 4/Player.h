#pragma once
#include "Entity.h"

// The player is now just an Entity with the player's glyph, colour and health.
// It inherits position and tryMove from Entity, so the movement code written
// back in Part 2 still works unchanged — it just lives in the base class now.
struct Player : Entity
{
    Player()
    {
        glyph = '@';
        color = { 255, 230, 150, 255 };   // bright gold
        hp = maxHp = 30;
    }
};
