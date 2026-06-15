#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <random>
#include <string>
#include "GlyphCache.h"
#include "Player.h"
#include "Monster.h"
#include "DijkstraMap.h"
#include "Item.h"
#include "Level.h"

enum class GameState { Playing, Inventory, Dead };

class Game
{
public:
    Game(SDL_Window* window, SDL_Renderer* sdl, GlyphCache& glyphs);
    void run();

private:
    Level&    cur() { return m_levels[m_depth]; }     // the current floor

    void      newGame();
    void      descend();
    void      playerAct(int dx, int dy);
    void      endPlayerTurn();
    void      monstersAct();
    void      attack(Entity& attacker, Entity& defender);
    void      tickStatuses(Entity& e);
    Monster*  monsterAt(int x, int y);

    void        pickUpHere();
    void        useItem(int slot);
    void        quaff(PotionType t);
    void        equip(const Item& it);
    void        recomputeStats();
    std::string potionName(PotionType t) const;
    std::string itemLabel(const Item& it) const;

    void render();
    void drawTile(int col, int row, const Tile& t);
    void drawCentred(int row, const char* text, SDL_Color color);
    void drawLeft(int col, int row, const char* text, SDL_Color color);
    void drawDeathScreen();
    void drawInventory();
    void updateTitle();

    SDL_Window*   m_window;
    SDL_Renderer* m_sdl;
    GlyphCache&   m_glyphs;

    std::vector<Level> m_levels;     // one per floor; index m_depth is current
    int                m_depth = 0;

    Player               m_player;
    std::vector<Item>    m_inventory;   // carried gear travels with the player
    DijkstraMap          m_toPlayer;
    std::mt19937         m_rng{ std::random_device{}() };
    GameState            m_state = GameState::Playing;

    const char* m_appearance[(int)PotionType::Count] = {};
    bool        m_identified[(int)PotionType::Count] = {};

    int  m_strengthBonus = 0;
    bool m_hasWeapon = false;  Item m_weapon{};
    bool m_hasArmour = false;  Item m_armour{};
};
