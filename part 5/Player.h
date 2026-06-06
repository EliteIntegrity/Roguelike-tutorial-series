#pragma once
#include "Entity.h"

struct Player : Entity
{
    Player()
    {
        glyph = '@';
        color = { 255, 230, 150, 255 };   // bright gold
        name  = "You";
        hp = maxHp = 30;
        power = 5;
    }
};
