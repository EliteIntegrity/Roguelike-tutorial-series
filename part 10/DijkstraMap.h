#pragma once
#include "Map.h"

// ---------------------------------------------------------------------------
//  A Dijkstra (distance) map. dist[y][x] holds the number of steps from the
//  goal to that tile, flooded across floor tiles only. Because every step
//  costs 1, this is really a breadth-first flood — but "Dijkstra map" is the
//  name the roguelike world uses for the technique.
//
//  A monster follows the shortest path to the goal by always stepping to the
//  lowest-valued neighbour ("rolling downhill"). Step to the HIGHEST-valued
//  neighbour instead and it flees — the same map, used in reverse.
// ---------------------------------------------------------------------------
class DijkstraMap
{
public:
    static const int UNREACHABLE = 1 << 30;

    int dist[MAP_HEIGHT][MAP_WIDTH];

    // Flood outward from a single goal (e.g. the player's tile).
    void compute(const Map& map, int goalX, int goalY);

    int at(int x, int y) const { return dist[y][x]; }
};
