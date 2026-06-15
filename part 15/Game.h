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

enum class GameState { ClassSelect, Playing, Inventory, Dead };
enum class PlayerClass { Fighter, Rogue, Mage };

class Game
{
public:
    Game(SDL_Window* window, SDL_Renderer* sdl, GlyphCache& glyphs);
    void run();

private:
    Level&    cur() { return m_levels[m_depth]; }

    void      newGame(PlayerClass cls);
    void      descend();
    void      playerAct(int dx, int dy);
    void      endPlayerTurn();
    void      monstersAct();
    void      attack(Entity& attacker, Entity& defender);
    void      tickStatuses(Entity& e);
    Monster*  monsterAt(int x, int y);
    Monster*  nearestVisibleMonster();

    void        pickUpHere();
    void        useItem(int slot);
    void        quaff(PotionType t);
    void        readScroll(ScrollType t);
    void        equip(const Item& it);
    void        recomputeStats();
    std::string potionName(PotionType t) const;
    std::string scrollName(ScrollType t) const;
    std::string itemLabel(const Item& it) const;

    Item makeWeapon(const char* name, int bonus);
    Item makeArmour(const char* name, int bonus);
    Item makeScroll(ScrollType s);
    Item makePotion(PotionType p);

    // --- persistence (Part 15) ---
    void save(const char* path);
    bool load(const char* path);

    void render();
    void drawTile(int col, int row, const Tile& t);
    void drawCentred(int row, const char* text, SDL_Color color);
    void drawLeft(int col, int row, const char* text, SDL_Color color);
    void drawClassSelect();
    void drawDeathScreen();
    void drawInventory();
    void updateTitle();
    void flash(const char* msg);            // brief title-bar notice

    SDL_Window*   m_window;
    SDL_Renderer* m_sdl;
    GlyphCache&   m_glyphs;

    std::vector<Level> m_levels;
    int                m_depth = 0;

    Player               m_player;
    std::vector<Item>    m_inventory;
    DijkstraMap          m_toPlayer;
    std::mt19937         m_rng{ std::random_device{}() };
    GameState            m_state = GameState::ClassSelect;
    PlayerClass          m_class = PlayerClass::Fighter;

    // identification — appearances stored as indices so they serialise trivially
    int  m_potionAppearIdx[(int)PotionType::Count] = {};
    bool m_potionIdent     [(int)PotionType::Count] = {};
    int  m_scrollAppearIdx [(int)ScrollType::Count] = {};
    bool m_scrollIdent      [(int)ScrollType::Count] = {};

    int  m_basePower  = 5;
    int  m_baseMaxHp  = 30;
    int  m_strengthBonus = 0;
    int  m_magicBonus    = 0;
    bool m_hasWeapon = false;  Item m_weapon{};
    bool m_hasArmour = false;  Item m_armour{};
};
