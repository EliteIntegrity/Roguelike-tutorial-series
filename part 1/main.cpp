#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Map.h"
#include "GlyphCache.h"

// RobotoMono-Light ships in the project folder. Swap in Regular or Medium for
// heavier strokes if you prefer — only this one line changes.
static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

// Palette. Floors are dim dots so the brighter walls read as structure.
static const SDL_Color BG    = {  12,  12,  16, 255 };
static const SDL_Color WALL  = { 130, 120, 150, 255 };
static const SDL_Color FLOOR = {  72,  66,  60, 255 };

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())               { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 1: Map Generation",
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

    Map map;
    map.generate();

    // Draw the whole map as ASCII. Wrapped in a lambda so we can call it both
    // before the loop and whenever something changes inside it.
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

        SDL_RenderPresent(sdl);
    };

    draw();   // show the first dungeon immediately

    // The map only changes when the player presses Space, so there is no
    // reason to spin a render loop. SDL_WaitEvent sleeps until an event
    // arrives, dropping CPU use to near zero while idle — exactly what a
    // turn-based game wants. We will build movement on top of this in Part 2.
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
                dirty = true;             // uncovered — repaint
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
                if (event.key.scancode == SDL_SCANCODE_SPACE)  { map.generate(); dirty = true; }
                break;
        }

        if (dirty) draw();
    }

    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
