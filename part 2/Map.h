#pragma once
#include <vector>
#include <memory>
#include <random>

// ---------------------------------------------------------------------------
//  Grid and window dimensions.
//  64 columns x 36 rows of 20px cells == a clean 1280 x 720 (16:9) window.
// ---------------------------------------------------------------------------
const int MAP_WIDTH  = 64;
const int MAP_HEIGHT = 36;
const int TILE_SIZE  = 20;

enum class TileType { Wall, Floor };

// A single map cell. It is deliberately a struct, not a bare enum, because it
// grows over the series: Part 3 adds `visible` and `explored` flags for field
// of view. Establishing the struct now means no refactor later.
struct Tile
{
    TileType type = TileType::Wall;
};

// A rectangular room: top-left corner (x, y) plus width and height.
struct Room
{
    int x = 0, y = 0, w = 0, h = 0;
    int centreX() const { return x + w / 2; }
    int centreY() const { return y + h / 2; }
};

// The dungeon. Pure data + a BSP generator — it knows nothing about SDL.
class Map
{
public:
    Tile              tiles[MAP_HEIGHT][MAP_WIDTH];
    std::vector<Room> rooms;

    // Build a fresh dungeon. Pass a fixed seed to reproduce a layout, or 0
    // (the default) to pick a random one from std::random_device.
    void generate(unsigned int seed = 0);

private:
    std::mt19937 m_rng;

    // One node of the binary space partition tree. Private and nested because
    // nothing outside Map needs to know the generator works this way.
    struct BSPNode
    {
        int x = 0, y = 0, w = 0, h = 0;      // bounds of this partition
        std::unique_ptr<BSPNode> left;        // null at leaf nodes
        std::unique_ptr<BSPNode> right;
        Room room{};
        bool hasRoom = false;
    };

    void split       (BSPNode& node, int depth);
    void buildRooms  (BSPNode& node);
    void connectRooms(BSPNode& node);

    void carveHLine(int x1, int x2, int y);
    void carveVLine(int y1, int y2, int x);
    void carveTile (int x,  int y,  TileType type);

    bool coinFlip() { return (m_rng() & 1) == 0; }
};
