#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <random>
#include <string>
#include "Map.h"
#include "GlyphCache.h"
#include "Player.h"
#include "Monster.h"
#include "DijkstraMap.h"
#include "Item.h"

enum class GameState { Playing, Inventory, Dead };

class Game
{
public:
    Game(SDL_Window* window, SDL_Renderer* sdl, GlyphCache& glyphs);
    void run();

private:
    void      newGame();
    void      playerAct(int dx, int dy);
    void      monstersAct();
    void      attack(Entity& attacker, Entity& defender);
    Monster*  monsterAt(int x, int y);

    // items / identification
    void        pickUpHere();
    void        quaff(int slot);            // drink inventory item, takes a turn
    void        applyPotion(PotionType t);
    std::string potionName(PotionType t) const;   // appearance, or true name if identified

    void render();
    void drawTile(int col, int row, const Tile& t);
    void drawCentred(int row, const char* text, SDL_Color color);
    void drawDeathScreen();
    void drawInventory();
    void updateTitle();

    SDL_Window*   m_window;
    SDL_Renderer* m_sdl;
    GlyphCache&   m_glyphs;

    Map                  m_map;
    Player               m_player;
    std::vector<Monster> m_monsters;
    std::vector<Item>    m_items;            // on the floor
    std::vector<PotionType> m_inventory;     // carried
    DijkstraMap          m_toPlayer;
    std::mt19937         m_rng{ std::random_device{}() };
    GameState            m_state = GameState::Playing;

    // identification state (per potion type, reset each game)
    const char* m_appearance[(int)PotionType::Count] = {};
    bool        m_identified[(int)PotionType::Count] = {};
};
