#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Map.h"
#include "GlyphCache.h"
#include "Game.h"

static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

// main() now has exactly one job: bring SDL up, hand a window, renderer and
// glyph cache to a Game, run it, and tear SDL down. Everything else lives in
// the Game class. This is the whole point of the refactor.
int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())               { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 6: The Game Class",
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

    {
        Game game(window, sdl, glyphs);
        game.run();
    }   // game is destroyed here, before we tear down SDL

    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
