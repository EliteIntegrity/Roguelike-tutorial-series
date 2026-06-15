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
    Level&    cur() { return m_levels[m_depth]; }

    void      newGame();
    void      descend();
    void      playerAct(int dx, int dy);
    void      endPlayerTurn();
    void      monstersAct();
    void      attack(Entity& attacker, Entity& defender);
    void      tickStatuses(Entity& e);
    Monster*  monsterAt(int x, int y);
    Monster*  nearestVisibleMonster();          // for ranged magic targeting

    void        pickUpHere();
    void        useItem(int slot);
    void        quaff(PotionType t);
    void        readScroll(ScrollType t);       // ranged / area magic
    void        equip(const Item& it);
    void        recomputeStats();
    std::string potionName(PotionType t) const;
    std::string scrollName(ScrollType t) const;
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

    std::vector<Level> m_levels;
    int                m_depth = 0;

    Player               m_player;
    std::vector<Item>    m_inventory;
    DijkstraMap          m_toPlayer;
    std::mt19937         m_rng{ std::random_device{}() };
    GameState            m_state = GameState::Playing;

    // identification
    const char* m_potionAppear[(int)PotionType::Count] = {};
    bool        m_potionIdent [(int)PotionType::Count] = {};
    const char* m_scrollAppear[(int)ScrollType::Count] = {};
    bool        m_scrollIdent [(int)ScrollType::Count] = {};

    // bonuses
    int  m_strengthBonus = 0;
    int  m_magicBonus    = 0;            // added to scroll damage (character classes use it)
    bool m_hasWeapon = false;  Item m_weapon{};
    bool m_hasArmour = false;  Item m_armour{};
};
