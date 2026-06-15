#pragma once
#include <SDL3/SDL.h>

// Amulet is the win-condition item — collect three of them in the deep floors.
enum class ItemCategory { Potion, Weapon, Armour, Scroll, Amulet };
enum class PotionType    { Healing, Poison, Strength, Regen, Count };
enum class ScrollType    { Lightning, Fireball, Count };

struct Item
{
    ItemCategory category = ItemCategory::Potion;

    PotionType   potion   = PotionType::Healing;
    ScrollType   scroll   = ScrollType::Lightning;
    int          bonus    = 0;
    const char*  name     = "";

    char         glyph    = '!';
    SDL_Color    color    = { 200, 120, 220, 255 };

    int          x = 0, y = 0;
};
