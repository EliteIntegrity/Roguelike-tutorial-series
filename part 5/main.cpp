#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <random>
#include <cstdlib>
#include "Map.h"
#include "FOV.h"
#include "GlyphCache.h"
#include "Player.h"
#include "Monster.h"

static const char* FONT_PATH = "RobotoMono-Light.ttf";
static const float FONT_PT   = 20.0f;

static const SDL_Color BG = { 12, 12, 16, 255 };

static const SDL_Color WALL_LIT  = { 150, 140, 175, 255 };
static const SDL_Color FLOOR_LIT = { 100,  92,  80, 255 };
static const SDL_Color WALL_MEM  = {  58,  54,  72, 255 };
static const SDL_Color FLOOR_MEM = {  40,  37,  33, 255 };

static const SDL_Color DEAD_FRAME = { 150,  60,  60, 255 };
static const SDL_Color DEAD_TITLE = { 220,  90,  90, 255 };
static const SDL_Color DEAD_TEXT  = { 210, 200, 205, 255 };
static const SDL_Color DEAD_HINT  = { 120, 112, 112, 255 };

struct MonsterKind { char glyph; SDL_Color color; int hp; int power; const char* name; };

static const MonsterKind KINDS[] = {
    { 'r', { 150, 150, 160, 255 }, 3, 1, "the rat"    },
    { 'k', { 110, 200, 120, 255 }, 5, 2, "the kobold" },
    { 'g', { 120, 170, 230, 255 }, 8, 3, "the goblin" },
};
static const int NUM_KINDS     = (int)(sizeof(KINDS) / sizeof(KINDS[0]));
static const int MONSTER_COUNT = 8;

static int sgn(int v) { return (v > 0) - (v < 0); }

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    if (!TTF_Init())               { SDL_Log("TTF_Init: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Window*   window = nullptr;
    SDL_Renderer* sdl    = nullptr;
    if (!SDL_CreateWindowAndRenderer(
            "Roguelike - Part 5: Melee Combat",
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

    std::mt19937 rng{ std::random_device{}() };

    Map                  map;
    Player               player;
    std::vector<Monster> monsters;
    bool                 dead = false;

    auto updateTitle = [&]()
    {
        char buf[96];
        SDL_snprintf(buf, sizeof(buf), "Roguelike - Part 5   |   HP: %d / %d",
                     player.hp, player.maxHp);
        SDL_SetWindowTitle(window, buf);
    };

    auto newGame = [&]()
    {
        map.generate();
        player.x  = map.rooms[0].centreX();
        player.y  = map.rooms[0].centreY();
        player.hp = player.maxHp;
        player.alive = true;
        dead = false;
        FOV::compute(map, player.x, player.y);

        monsters.clear();
        int n = (int)map.rooms.size();
        for (int i = 1; i < n && (int)monsters.size() < MONSTER_COUNT; ++i)
        {
            const MonsterKind& k = KINDS[std::uniform_int_distribution<int>(0, NUM_KINDS - 1)(rng)];
            Monster m;
            m.x = map.rooms[i].centreX();
            m.y = map.rooms[i].centreY();
            m.glyph = k.glyph;  m.color = k.color;
            m.hp = m.maxHp = k.hp;
            m.power = k.power;  m.name  = k.name;
            monsters.push_back(m);
        }
        updateTitle();
    };

    // A living monster on a tile, or nullptr.
    auto monsterAt = [&](int x, int y) -> Monster*
    {
        for (Monster& m : monsters)
            if (m.isAlive() && m.x == x && m.y == y) return &m;
        return nullptr;
    };

    // One creature strikes another. Logs the blow; marks the loser dead.
    auto attack = [&](Entity& atk, Entity& def)
    {
        def.hp -= atk.power;
        SDL_Log("%s hits %s for %d. (%s: %d hp)", atk.name, def.name, atk.power,
                def.name, def.hp > 0 ? def.hp : 0);
        if (def.hp <= 0)
        {
            def.hp = 0;
            def.alive = false;
            SDL_Log("%s dies.", def.name);
        }
    };

    // Every monster takes its turn after the player acts.
    auto monstersAct = [&]()
    {
        for (Monster& m : monsters)
        {
            if (!m.isAlive()) continue;

            // It only hunts when its tile is lit — i.e. when you can see it,
            // it can see you. Otherwise it wanders, unseen, in the dark.
            if (!map.tiles[m.y][m.x].visible)
            {
                m.wander(map, rng);
                continue;
            }

            int ddx = player.x - m.x;
            int ddy = player.y - m.y;

            // Adjacent to the player? Strike. (Four-directional, like the player.)
            if (std::abs(ddx) + std::abs(ddy) == 1)
            {
                attack(m, player);
                continue;
            }

            // Otherwise close the distance: step along the longer axis first,
            // fall back to the other axis if that way is blocked.
            int sx = (std::abs(ddx) >= std::abs(ddy)) ? sgn(ddx) : 0;
            int sy = (sx == 0) ? sgn(ddy) : 0;

            auto stepIfFree = [&](int dx, int dy) -> bool
            {
                if (dx == 0 && dy == 0) return false;
                int nx = m.x + dx, ny = m.y + dy;
                if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) return false;
                if (map.tiles[ny][nx].type != TileType::Floor) return false;
                if (nx == player.x && ny == player.y) return false;  // never stack on player
                if (monsterAt(nx, ny)) return false;                 // or on each other
                m.x = nx; m.y = ny; return true;
            };

            if (!stepIfFree(sx, sy))
                stepIfFree(sgn(ddx) - sx, sgn(ddy) - sy);   // try the other axis
        }
    };

    // -- rendering ---------------------------------------------------------

    auto drawCentred = [&](int row, const char* text, SDL_Color color)
    {
        int len = (int)SDL_strlen(text);
        int startCol = (MAP_WIDTH - len) / 2;
        for (int i = 0; i < len; ++i)
            glyphs.drawGlyph(startCol + i, row, text[i], color);
    };

    auto drawDeathScreen = [&]()
    {
        SDL_SetRenderDrawColor(sdl, BG.r, BG.g, BG.b, BG.a);
        SDL_RenderClear(sdl);

        const int boxW = 40, boxH = 9;
        const int x0 = (MAP_WIDTH  - boxW) / 2;
        const int y0 = (MAP_HEIGHT - boxH) / 2;
        for (int i = 0; i < boxW; ++i)
        {
            glyphs.drawGlyph(x0 + i, y0,            '=', DEAD_FRAME);
            glyphs.drawGlyph(x0 + i, y0 + boxH - 1, '=', DEAD_FRAME);
        }
        for (int j = 0; j < boxH; ++j)
        {
            glyphs.drawGlyph(x0,            y0 + j, '|', DEAD_FRAME);
            glyphs.drawGlyph(x0 + boxW - 1, y0 + j, '|', DEAD_FRAME);
        }

        drawCentred(y0 + 2, "Y O U   D I E D", DEAD_TITLE);
        drawCentred(y0 + 4, "The dungeon claims another.", DEAD_TEXT);
        drawCentred(y0 + 6, "SPACE  try again        ESC  quit", DEAD_HINT);

        SDL_RenderPresent(sdl);
    };

    auto drawTile = [&](int col, int row, const Tile& t)
    {
        if (!t.explored) return;
        if (t.visible)
        {
            if (t.type == TileType::Floor) glyphs.drawGlyph(col, row, '.', FLOOR_LIT);
            else                           glyphs.drawGlyph(col, row, '#', WALL_LIT);
        }
        else
        {
            if (t.type == TileType::Floor) glyphs.drawGlyph(col, row, '.', FLOOR_MEM);
            else                           glyphs.drawGlyph(col, row, '#', WALL_MEM);
        }
    };

    auto draw = [&]()
    {
        if (dead) { drawDeathScreen(); return; }

        SDL_SetRenderDrawColor(sdl, BG.r, BG.g, BG.b, BG.a);
        SDL_RenderClear(sdl);

        for (int row = 0; row < MAP_HEIGHT; ++row)
            for (int col = 0; col < MAP_WIDTH; ++col)
                drawTile(col, row, map.tiles[row][col]);

        for (const Monster& m : monsters)
            if (m.isAlive() && map.tiles[m.y][m.x].visible)
                glyphs.drawGlyph(m.x, m.y, m.glyph, m.color);

        glyphs.drawGlyph(player.x, player.y, player.glyph, player.color);

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
                if (dead)
                {
                    if (event.key.scancode == SDL_SCANCODE_SPACE)  { newGame(); dirty = true; }
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
                    break;
                }

                int dx = 0, dy = 0;
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
                {
                    bool tookTurn = false;

                    // Bump-to-attack: if a monster is where we're heading, hit
                    // it instead of moving. Otherwise, move if the floor allows.
                    Monster* target = monsterAt(player.x + dx, player.y + dy);
                    if (target)
                    {
                        attack(player, *target);
                        tookTurn = true;
                    }
                    else if (player.tryMove(dx, dy, map))
                    {
                        FOV::compute(map, player.x, player.y);
                        tookTurn = true;
                    }

                    if (tookTurn)
                    {
                        monstersAct();
                        updateTitle();
                        if (player.hp <= 0) dead = true;
                        dirty = true;
                    }
                }
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
