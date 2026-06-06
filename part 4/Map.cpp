#include "Map.h"
#include <algorithm>

// Tuning constants for the 64 x 36 grid.
static constexpr int MAX_DEPTH    = 4;   // up to 2^4 = 16 leaf partitions
static constexpr int MIN_LEAF_W   = 12;  // a partition narrower than this won't split vertically
static constexpr int MIN_LEAF_H   = 9;   // a partition shorter than this won't split horizontally
static constexpr int ROOM_PADDING = 1;   // empty border kept inside each partition
static constexpr int ROOM_MIN     = 4;   // smallest room edge length

void Map::generate(unsigned int seed)
{
    // Reset the whole grid to solid rock. Assigning a default-constructed Tile
    // clears every field — including the visible/explored flags added in
    // Part 3 — so regenerating mid-game never leaves stale state behind.
    for (auto& row : tiles)
        for (auto& tile : row)
            tile = Tile{};
    rooms.clear();

    if (seed == 0)
    {
        std::random_device rd;
        seed = rd();
    }
    m_rng.seed(seed);

    BSPNode root;
    root.x = 0;          root.y = 0;
    root.w = MAP_WIDTH;  root.h = MAP_HEIGHT;

    split(root, 0);        // phase 1: recursively partition the space
    buildRooms(root);      // phase 2: carve one room into each leaf
    connectRooms(root);    // phase 3: join sibling rooms with corridors
}

// ---------------------------------------------------------------------------
//  Phase 1 — recursively split the partition tree.
// ---------------------------------------------------------------------------
void Map::split(BSPNode& node, int depth)
{
    if (depth >= MAX_DEPTH) return;

    bool canSplitH = (node.h >= MIN_LEAF_H * 2);
    bool canSplitV = (node.w >= MIN_LEAF_W * 2);
    if (!canSplitH && !canSplitV) return;

    // Prefer to cut across the longer axis so partitions stay roughly square;
    // flip a coin to break ties.
    bool horizontal;
    if      (!canSplitV) horizontal = true;
    else if (!canSplitH) horizontal = false;
    else    horizontal = (node.h > node.w) || (node.h == node.w && coinFlip());

    node.left  = std::make_unique<BSPNode>();
    node.right = std::make_unique<BSPNode>();

    if (horizontal)
    {
        int splitY = std::uniform_int_distribution<int>(
            node.y + MIN_LEAF_H, node.y + node.h - MIN_LEAF_H)(m_rng);

        node.left->x  = node.x;  node.left->y  = node.y;
        node.left->w  = node.w;  node.left->h  = splitY - node.y;

        node.right->x = node.x;  node.right->y = splitY;
        node.right->w = node.w;  node.right->h = (node.y + node.h) - splitY;
    }
    else
    {
        int splitX = std::uniform_int_distribution<int>(
            node.x + MIN_LEAF_W, node.x + node.w - MIN_LEAF_W)(m_rng);

        node.left->x  = node.x;          node.left->y  = node.y;
        node.left->w  = splitX - node.x; node.left->h  = node.h;

        node.right->x = splitX;          node.right->y = node.y;
        node.right->w = (node.x + node.w) - splitX;  node.right->h = node.h;
    }

    split(*node.left,  depth + 1);
    split(*node.right, depth + 1);
}

// ---------------------------------------------------------------------------
//  Phase 2 — carve a room into every leaf partition.
// ---------------------------------------------------------------------------
void Map::buildRooms(BSPNode& node)
{
    if (node.left)                  // internal node — recurse into children
    {
        buildRooms(*node.left);
        buildRooms(*node.right);
        return;
    }

    // Leaf: the room must fit inside the partition with ROOM_PADDING on
    // every side. The padding guarantees a wall gap between adjacent rooms.
    int maxW = node.w - ROOM_PADDING * 2;
    int maxH = node.h - ROOM_PADDING * 2;
    if (maxW < ROOM_MIN || maxH < ROOM_MIN) return;   // partition too small

    int rw = std::uniform_int_distribution<int>(ROOM_MIN, maxW)(m_rng);
    int rh = std::uniform_int_distribution<int>(ROOM_MIN, maxH)(m_rng);
    int rx = node.x + ROOM_PADDING + std::uniform_int_distribution<int>(0, maxW - rw)(m_rng);
    int ry = node.y + ROOM_PADDING + std::uniform_int_distribution<int>(0, maxH - rh)(m_rng);

    node.room    = { rx, ry, rw, rh };
    node.hasRoom = true;

    for (int y = ry; y < ry + rh; ++y)
        for (int x = rx; x < rx + rw; ++x)
            carveTile(x, y, TileType::Floor);

    rooms.push_back(node.room);
}

// ---------------------------------------------------------------------------
//  Phase 3 — connect each internal node's two subtrees with a corridor.
// ---------------------------------------------------------------------------
void Map::connectRooms(BSPNode& node)
{
    if (!node.left) return;          // leaf — nothing to connect

    connectRooms(*node.left);
    connectRooms(*node.right);

    // Walk randomly down each subtree to reach a leaf that actually has a
    // room. Picking randomly varies which rooms are directly linked, so the
    // corridors don't always follow the same spine through the dungeon.
    BSPNode* l = node.left.get();
    while (l->left) l = coinFlip() ? l->left.get() : l->right.get();

    BSPNode* r = node.right.get();
    while (r->left) r = coinFlip() ? r->left.get() : r->right.get();

    if (!l->hasRoom || !r->hasRoom) return;

    int x1 = l->room.centreX(), y1 = l->room.centreY();
    int x2 = r->room.centreX(), y2 = r->room.centreY();

    // L-shaped corridor — bend in a random corner.
    if (coinFlip())
    {
        carveHLine(x1, x2, y1);
        carveVLine(y1, y2, x2);
    }
    else
    {
        carveVLine(y1, y2, x1);
        carveHLine(x1, x2, y2);
    }
}

// ---------------------------------------------------------------------------
//  Carving helpers — every write is bounds-checked.
// ---------------------------------------------------------------------------
void Map::carveTile(int x, int y, TileType type)
{
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
        tiles[y][x].type = type;
}

void Map::carveHLine(int x1, int x2, int y)
{
    if (x1 > x2) std::swap(x1, x2);
    for (int x = x1; x <= x2; ++x)
        carveTile(x, y, TileType::Floor);
}

void Map::carveVLine(int y1, int y2, int x)
{
    if (y1 > y2) std::swap(y1, y2);
    for (int y = y1; y <= y2; ++y)
        carveTile(x, y, TileType::Floor);
}
