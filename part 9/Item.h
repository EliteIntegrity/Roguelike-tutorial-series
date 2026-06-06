#pragma once
#include <SDL3/SDL.h>

// Items are now first-class. A potion is drunk; a weapon or armour is worn.
// The inventory holds full Items, not just potion types.
enum class ItemCategory { Potion, Weapon, Armour };
enum class PotionType    { Healing, Poison, Strength, Count };

struct Item
{
    ItemCategory category = ItemCategory::Potion;

    PotionType   potion   = PotionType::Healing;   // when category == Potion
    int          bonus    = 0;                     // power (Weapon) or defense (Armour)
    const char*  name     = "";                    // true name for equipment

    char         glyph    = '!';
    SDL_Color    color    = { 200, 120, 220, 255 };

    int          x = 0;       // position while on the floor
    int          y = 0;
};
