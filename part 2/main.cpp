#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Map.h"
#include "GlyphCache.h"
#include "Player.h"

static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

static const SDL_Color BG     = {  12,  12,  16, 255 };
static const SDL_Color WALL   = { 130, 120, 150, 255 };
static const SDL_Color FLOOR  = {  72,  66,  60, 255 };
static const SDL_Color PLAYER = { 255, 230, 150, 255 };   // bright gold @

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())               { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 2: Player & Movement",
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

    // Start a fresh dungeon and drop the player in the centre of the first
    // room. The BSP tree is processed in a fixed order, so rooms[0] is always
    // a real, walkable room. Part 4 will choose a smarter spawn.
    auto newGame = [&]()
    {
        map.generate();
        player.x  = map.rooms[0].centreX();
        player.y  = map.rooms[0].centreY();
        player.hp = player.maxHp;
    };

    auto draw = [&]()
    {
        SDL_SetRenderDrawColor(sdl, BG.r, BG.g, BG.b, BG.a);
        SDL_RenderClear(sdl);

        for (int row = 0; row < MAP_HEIGHT; ++row)
            for (int col = 0; col < MAP_WIDTH; ++col)
            {
                if (map.tiles[row][col].type == TileType::Floor)
                    glyphs.drawGlyph(col, row, '.', FLOOR);
                else
                    glyphs.drawGlyph(col, row, '#', WALL);
            }

        // Player is drawn last so the @ always sits on top of its tile.
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

                // Scancodes map to physical key POSITIONS, so WASD stays in
                // the same place on AZERTY and other layouts. Use scancodes
                // for game controls; use key symbols only for text entry.
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
                    dirty = player.tryMove(dx, dy, map);   // false if blocked → no repaint
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
