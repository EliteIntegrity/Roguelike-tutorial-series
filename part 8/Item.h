#pragma once

// Potions are the items in this part. Each TYPE has a fixed effect, but a
// random APPEARANCE each game — you don't learn which "murky potion" heals and
// which poisons until you drink one (or identify it some other way).
enum class PotionType { Healing, Poison, Strength, Count };

// An item lying on the dungeon floor. Once picked up, only its `type` matters
// and it lives in the inventory as a PotionType.
struct Item
{
    PotionType type = PotionType::Healing;
    int x = 0;
    int y = 0;
};
