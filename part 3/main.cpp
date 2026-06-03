#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Map.h"
#include "FOV.h"
#include "GlyphCache.h"
#include "Player.h"

static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

static const SDL_Color BG     = {  12,  12,  16, 255 };
static const SDL_Color PLAYER = { 255, 230, 150, 255 };

// Two brightness tiers per tile type: lit (in current view) and remembered
// (explored but out of sight, drawn at roughly a quarter strength).
static const SDL_Color WALL_LIT  = { 150, 140, 175, 255 };
static const SDL_Color FLOOR_LIT = { 100,  92,  80, 255 };
static const SDL_Color WALL_MEM  = {  58,  54,  72, 255 };
static const SDL_Color FLOOR_MEM = {  40,  37,  33, 255 };

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())               { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 3: Field of View",
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

    Map    map;
    Player player;

    auto newGame = [&]()
    {
        map.generate();
        player.x  = map.rooms[0].centreX();
        player.y  = map.rooms[0].centreY();
        player.hp = player.maxHp;
        FOV::compute(map, player.x, player.y);   // light the starting area
    };

    // Choose a glyph + colour for one tile from its three possible states:
    // unexplored (skip), visible (lit), or remembered (dim).
    auto drawTile = [&](int col, int row, const Tile& t)
    {
        if (!t.explored) return;                 // never seen — leave as void

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

        glyphs.drawGlyph(player.x, player.y, '@', PLAYER);

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
                        FOV::compute(map, player.x, player.y);   // recompute after a real move
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
