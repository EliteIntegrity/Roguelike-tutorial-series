#pragma once
#include <SDL3/SDL.h>

enum class ItemCategory { Potion, Weapon, Armour };

// Poison and Regeneration now apply lasting STATUS effects rather than firing
// once — see Game::quaff.
enum class PotionType { Healing, Poison, Strength, Regen, Count };

struct Item
{
    ItemCategory category = ItemCategory::Potion;

    PotionType   potion   = PotionType::Healing;
    int          bonus    = 0;
    const char*  name     = "";

    char         glyph    = '!';
    SDL_Color    color    = { 200, 120, 220, 255 };

    int          x = 0, y = 0;
};
