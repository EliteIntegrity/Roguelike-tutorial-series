#pragma once
#include <vector>
#include <random>
#include "Map.h"
#include "Monster.h"
#include "Item.h"

// ---------------------------------------------------------------------------
//  A single dungeon floor. This is the Level extraction we planned at the
//  Interlude: the map, the monsters and the items now belong to a FLOOR, not
//  to the game globally. The Game owns a stack of these and swaps between them
//  as the player descends. Each Level keeps its own explored/visible state (in
//  its map), so a floor you return to looks exactly as you left it.
// ---------------------------------------------------------------------------
class Level
{
public:
    Map                  map;
    std::vector<Monster> monsters;
    std::vector<Item>    items;
    int                  stairsX = 0;     // the '>' down-stairs
    int                  stairsY = 0;

    // Build this floor. Deeper floors (higher depth) get more and tougher
    // monsters — the "depth scaling" half of the part.
    void generate(int depth, std::mt19937& rng);
};
