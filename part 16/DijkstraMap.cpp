#include "DijkstraMap.h"
#include <queue>
#include <utility>

void DijkstraMap::compute(const Map& map, int goalX, int goalY)
{
    // Start every tile at "infinitely far".
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            dist[y][x] = UNREACHABLE;

    if (map.tiles[goalY][goalX].type != TileType::Floor)
        return;   // goal isn't walkable — leave everything unreachable

    // Breadth-first flood from the goal. A queue visits tiles in order of
    // distance, so the first time we reach a tile is via a shortest path.
    std::queue<std::pair<int, int>> frontier;
    dist[goalY][goalX] = 0;
    frontier.push({ goalX, goalY });

    static const int dx[4] = {  0,  0, -1, +1 };
    static const int dy[4] = { -1, +1,  0,  0 };

    while (!frontier.empty())
    {
        std::pair<int, int> cur = frontier.front();
        frontier.pop();
        int cx = cur.first, cy = cur.second;
        int next = dist[cy][cx] + 1;

        for (int i = 0; i < 4; ++i)
        {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (map.tiles[ny][nx].type != TileType::Floor) continue;
            if (next < dist[ny][nx])
            {
                dist[ny][nx] = next;
                frontier.push({ nx, ny });
            }
        }
    }
}
