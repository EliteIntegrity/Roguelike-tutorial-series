#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <random>
#include "Map.h"
#include "FOV.h"
#include "GlyphCache.h"
#include "Player.h"
#include "Monster.h"

static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

static const SDL_Color BG = { 12, 12, 16, 255 };

static const SDL_Color WALL_LIT  = { 150, 140, 175, 255 };
static const SDL_Color FLOOR_LIT = { 100,  92,  80, 255 };
static const SDL_Color WALL_MEM  = {  58,  54,  72, 255 };
static const SDL_Color FLOOR_MEM = {  40,  37,  33, 255 };

// A monster "kind" is just the look + health we stamp onto a fresh Monster.
struct MonsterKind { char glyph; SDL_Color color; int hp; };

static const MonsterKind KINDS[] = {
    { 'r', { 150, 150, 160, 255 }, 3 },   // rat
    { 'k', { 110, 200, 120, 255 }, 5 },   // kobold
    { 'g', { 120, 170, 230, 255 }, 8 },   // goblin
};
static const int NUM_KINDS    = (int)(sizeof(KINDS) / sizeof(KINDS[0]));
static const int MONSTER_COUNT = 8;

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())               { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 4: Monsters & AI",
            MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, 0,
            &window, &sdl))
    {
        SDL_Log("CreateWindowAndRenderer: %s", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    GlyphCache glyphs(sdl, FONT_PATH, FONT_PT);
    if (!glyphs.ok())
    {
        SDL_DestroyRenderer(sdl); SDL_DestroyWindow(window);
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    std::mt19937 rng{ std::random_device{}() };

    Map                  map;
    Player               player;
    std::vector<Monster> monsters;

    auto newGame = [&]()
    {
        map.generate();
        player.x  = map.rooms[0].centreX();
        player.y  = map.rooms[0].centreY();
        player.hp = player.maxHp;
        FOV::compute(map, player.x, player.y);

        // One monster per room, starting at room 1 so none share the player's
        // spawn. Each gets a random kind.
        monsters.clear();
        int n = (int)map.rooms.size();
        for (int i = 1; i < n && (int)monsters.size() < MONSTER_COUNT; ++i)
        {
            const MonsterKind& k = KINDS[std::uniform_int_distribution<int>(0, NUM_KINDS - 1)(rng)];
            Monster m;
            m.x = map.rooms[i].centreX();
            m.y = map.rooms[i].centreY();
            m.glyph = k.glyph;
            m.color = k.color;
            m.hp = m.maxHp = k.hp;
            monsters.push_back(m);
        }
    };

    auto drawTile = [&](int col, int row, const Tile& t)
    {
        if (!t.explored) return;
        if (t.visible)
        {
            if (t.type == TileType::Floor) glyphs.drawGlyph(col, row, '.', FLOOR_LIT);
            else                           glyphs.drawGlyph(col, row, '#', WALL_LIT);
        }
        else
        {
            if (t.type == TileType::Floor) glyphs.drawGlyph(col, row, '.', FLOOR_MEM);
            else                           glyphs.drawGlyph(col, row, '#', WALL_MEM);
        }
    };

    auto draw = [&]()
    {
        SDL_SetRenderDrawColor(sdl, BG.r, BG.g, BG.b, BG.a);
        SDL_RenderClear(sdl);

        for (int row = 0; row < MAP_HEIGHT; ++row)
            for (int col = 0; col < MAP_WIDTH; ++col)
                drawTile(col, row, map.tiles[row][col]);

        // Monsters are only drawn when the player can currently see their tile.
        // Leave the dungeon out of sight, and so are its inhabitants.
        for (const Monster& m : monsters)
            if (m.isAlive() && map.tiles[m.y][m.x].visible)
                glyphs.drawGlyph(m.x, m.y, m.glyph, m.color);

        glyphs.drawGlyph(player.x, player.y, player.glyph, player.color);

        SDL_RenderPresent(sdl);
    };

    newGame();
    draw();

    bool running = true;
    SDL_Event event;

    while (running)
    {
        SDL_WaitEvent(&event);
        bool dirty = false;

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_WINDOW_EXPOSED:
                dirty = true;
                break;

            case SDL_EVENT_KEY_DOWN:
            {
                int dx = 0, dy = 0;
                switch (event.key.scancode)
                {
                    case SDL_SCANCODE_UP:    case SDL_SCANCODE_W: dy = -1; break;
                    case SDL_SCANCODE_DOWN:  case SDL_SCANCODE_S: dy = +1; break;
                    case SDL_SCANCODE_LEFT:  case SDL_SCANCODE_A: dx = -1; break;
                    case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D: dx = +1; break;
                    case SDL_SCANCODE_SPACE:  newGame(); dirty = true; break;
                    case SDL_SCANCODE_ESCAPE: running = false;         break;
                    default: break;
                }

                if (dx != 0 || dy != 0)
                {
                    if (player.tryMove(dx, dy, map))
                    {
                        FOV::compute(map, player.x, player.y);

                        // The player took a turn, so now every monster takes one.
                        for (Monster& m : monsters)
                            if (m.isAlive())
                                m.wander(map, rng);

                        dirty = true;
                    }
                }
                break;
            }
        }

        if (dirty) draw();
    }

    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
