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

    // items
    void        pickUpHere();
    void        useItem(int slot);          // quaff a potion or equip gear; takes a turn
    void        quaff(PotionType t);
    void        equip(const Item& it);
    void        recomputeStats();
    std::string potionName(PotionType t) const;
    std::string itemLabel(const Item& it) const;
    Item        randomItem();

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

    Map                  m_map;
    Player               m_player;
    std::vector<Monster> m_monsters;
    std::vector<Item>    m_items;        // on the floor
    std::vector<Item>    m_inventory;    // carried
    DijkstraMap          m_toPlayer;
    std::mt19937         m_rng{ std::random_device{}() };
    GameState            m_state = GameState::Playing;

    // identification
    const char* m_appearance[(int)PotionType::Count] = {};
    bool        m_identified[(int)PotionType::Count] = {};

    // equipment + stat bonuses (the player's power/defense are derived from these)
    int  m_strengthBonus = 0;            // from strength potions
    bool m_hasWeapon = false;  Item m_weapon{};
    bool m_hasArmour = false;  Item m_armour{};
};
