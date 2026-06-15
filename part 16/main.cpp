#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Map.h"
#include "GlyphCache.h"
#include "Game.h"

static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

int main(int argc, char* argv[])
{
    // SDL_INIT_AUDIO is new this part — for the synthesised blips.
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())                                { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    // The window is HUD_ROWS taller than the map now, to hold the status line
    // and the on-screen message log.
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 16: UI & Sound",
            MAP_WIDTH * TILE_SIZE, (MAP_HEIGHT + HUD_ROWS) * TILE_SIZE, 0,
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

    {
        Game game(window, sdl, glyphs);
        game.run();
    }

    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
