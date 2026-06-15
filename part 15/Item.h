#pragma once
#include <SDL3/SDL.h>

enum class ItemCategory { Potion, Weapon, Armour, Scroll };
enum class PotionType    { Healing, Poison, Strength, Regen, Count };

// Scrolls are this part's magic: read from the inventory to strike at range.
enum class ScrollType { Lightning, Fireball, Count };

struct Item
{
    ItemCategory category = ItemCategory::Potion;

    PotionType   potion   = PotionType::Healing;   // when category == Potion
    ScrollType   scroll   = ScrollType::Lightning; // when category == Scroll
    int          bonus    = 0;                     // power (Weapon) or defense (Armour)
    const char*  name     = "";                    // true name for equipment

    char         glyph    = '!';
    SDL_Color    color    = { 200, 120, 220, 255 };

    int          x = 0, y = 0;
};
