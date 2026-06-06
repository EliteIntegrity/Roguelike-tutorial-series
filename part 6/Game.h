#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <random>
#include "Map.h"
#include "GlyphCache.h"
#include "Player.h"
#include "Monster.h"

// The whole game used to live as loose state and lambdas inside main(). It now
// lives here. `GameState` replaces the ad-hoc bools that tracked whether the
// player was alive — and is the natural place to add menus, a win state, and
// more as the series grows.
enum class GameState { Playing, Dead };

class Game
{
public:
    Game(SDL_Window* window, SDL_Renderer* sdl, GlyphCache& glyphs);

    // Owns the event loop. Returns when the player quits.
    void run();

private:
    // --- turn logic ---
    void      newGame();
    void      playerAct(int dx, int dy);   // move or bump-attack, then monsters
    void      monstersAct();
    void      attack(Entity& attacker, Entity& defender);
    Monster*  monsterAt(int x, int y);

    // --- rendering ---
    void render();
    void drawTile(int col, int row, const Tile& t);
    void drawCentred(int row, const char* text, SDL_Color color);
    void drawDeathScreen();
    void updateTitle();

    // --- owned state (was scattered through main) ---
    SDL_Window*   m_window;
    SDL_Renderer* m_sdl;
    GlyphCache&   m_glyphs;

    Map                  m_map;
    Player               m_player;
    std::vector<Monster> m_monsters;
    std::mt19937         m_rng{ std::random_device{}() };
    GameState            m_state = GameState::Playing;
};
