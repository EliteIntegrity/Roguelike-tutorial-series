#include "Game.h"
#include "FOV.h"
#include <cstdlib>

// Palette — unchanged from Part 5, now scoped to this translation unit.
namespace {
    const SDL_Color BG = { 12, 12, 16, 255 };

    const SDL_Color WALL_LIT  = { 150, 140, 175, 255 };
    const SDL_Color FLOOR_LIT = { 100,  92,  80, 255 };
    const SDL_Color WALL_MEM  = {  58,  54,  72, 255 };
    const SDL_Color FLOOR_MEM = {  40,  37,  33, 255 };

    const SDL_Color DEAD_FRAME = { 150,  60,  60, 255 };
    const SDL_Color DEAD_TITLE = { 220,  90,  90, 255 };
    const SDL_Color DEAD_TEXT  = { 210, 200, 205, 255 };
    const SDL_Color DEAD_HINT  = { 120, 112, 112, 255 };

    struct MonsterKind { char glyph; SDL_Color color; int hp; int power; const char* name; };
    const MonsterKind KINDS[] = {
        { 'r', { 150, 150, 160, 255 }, 3, 1, "the rat"    },
        { 'k', { 110, 200, 120, 255 }, 5, 2, "the kobold" },
        { 'g', { 120, 170, 230, 255 }, 8, 3, "the goblin" },
    };
    const int NUM_KINDS     = (int)(sizeof(KINDS) / sizeof(KINDS[0]));
    const int MONSTER_COUNT = 8;

    int sgn(int v) { return (v > 0) - (v < 0); }
}

Game::Game(SDL_Window* window, SDL_Renderer* sdl, GlyphCache& glyphs)
    : m_window(window), m_sdl(sdl), m_glyphs(glyphs)
{
    newGame();
}

void Game::newGame()
{
    m_map.generate();
    m_player.x  = m_map.rooms[0].centreX();
    m_player.y  = m_map.rooms[0].centreY();
    m_player.hp = m_player.maxHp;
    m_player.alive = true;
    m_state = GameState::Playing;
    FOV::compute(m_map, m_player.x, m_player.y);

    m_monsters.clear();
    int n = (int)m_map.rooms.size();
    for (int i = 1; i < n && (int)m_monsters.size() < MONSTER_COUNT; ++i)
    {
        const MonsterKind& k = KINDS[std::uniform_int_distribution<int>(0, NUM_KINDS - 1)(m_rng)];
        Monster m;
        m.x = m_map.rooms[i].centreX();
        m.y = m_map.rooms[i].centreY();
        m.glyph = k.glyph;  m.color = k.color;
        m.hp = m.maxHp = k.hp;
        m.power = k.power;  m.name = k.name;
        m_monsters.push_back(m);
    }
    updateTitle();
}

Monster* Game::monsterAt(int x, int y)
{
    for (Monster& m : m_monsters)
        if (m.isAlive() && m.x == x && m.y == y) return &m;
    return nullptr;
}

void Game::attack(Entity& atk, Entity& def)
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
}

void Game::playerAct(int dx, int dy)
{
    bool tookTurn = false;

    Monster* target = monsterAt(m_player.x + dx, m_player.y + dy);
    if (target)
    {
        attack(m_player, *target);
        tookTurn = true;
    }
    else if (m_player.tryMove(dx, dy, m_map))
    {
        FOV::compute(m_map, m_player.x, m_player.y);
        tookTurn = true;
    }

    if (tookTurn)
    {
        monstersAct();
        updateTitle();
        if (m_player.hp <= 0) m_state = GameState::Dead;
    }
}

void Game::monstersAct()
{
    for (Monster& m : m_monsters)
    {
        if (!m.isAlive()) continue;

        if (!m_map.tiles[m.y][m.x].visible)
        {
            m.wander(m_map, m_rng);
            continue;
        }

        int ddx = m_player.x - m.x;
        int ddy = m_player.y - m.y;

        if (std::abs(ddx) + std::abs(ddy) == 1)
        {
            attack(m, m_player);
            continue;
        }

        int sx = (std::abs(ddx) >= std::abs(ddy)) ? sgn(ddx) : 0;
        int sy = (sx == 0) ? sgn(ddy) : 0;

        auto stepIfFree = [&](int dxx, int dyy) -> bool
        {
            if (dxx == 0 && dyy == 0) return false;
            int nx = m.x + dxx, ny = m.y + dyy;
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) return false;
            if (m_map.tiles[ny][nx].type != TileType::Floor) return false;
            if (nx == m_player.x && ny == m_player.y) return false;
            if (monsterAt(nx, ny)) return false;
            m.x = nx; m.y = ny; return true;
        };

        if (!stepIfFree(sx, sy))
            stepIfFree(sgn(ddx) - sx, sgn(ddy) - sy);
    }
}

void Game::updateTitle()
{
    char buf[96];
    SDL_snprintf(buf, sizeof(buf), "Roguelike - Part 6   |   HP: %d / %d",
                 m_player.hp, m_player.maxHp);
    SDL_SetWindowTitle(m_window, buf);
}

void Game::drawTile(int col, int row, const Tile& t)
{
    if (!t.explored) return;
    if (t.visible)
    {
        if (t.type == TileType::Floor) m_glyphs.drawGlyph(col, row, '.', FLOOR_LIT);
        else                           m_glyphs.drawGlyph(col, row, '#', WALL_LIT);
    }
    else
    {
        if (t.type == TileType::Floor) m_glyphs.drawGlyph(col, row, '.', FLOOR_MEM);
        else                           m_glyphs.drawGlyph(col, row, '#', WALL_MEM);
    }
}

void Game::drawCentred(int row, const char* text, SDL_Color color)
{
    int len = (int)SDL_strlen(text);
    int startCol = (MAP_WIDTH - len) / 2;
    for (int i = 0; i < len; ++i)
        m_glyphs.drawGlyph(startCol + i, row, text[i], color);
}

void Game::drawDeathScreen()
{
    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);

    const int boxW = 40, boxH = 9;
    const int x0 = (MAP_WIDTH  - boxW) / 2;
    const int y0 = (MAP_HEIGHT - boxH) / 2;
    for (int i = 0; i < boxW; ++i)
    {
        m_glyphs.drawGlyph(x0 + i, y0,            '=', DEAD_FRAME);
        m_glyphs.drawGlyph(x0 + i, y0 + boxH - 1, '=', DEAD_FRAME);
    }
    for (int j = 0; j < boxH; ++j)
    {
        m_glyphs.drawGlyph(x0,            y0 + j, '|', DEAD_FRAME);
        m_glyphs.drawGlyph(x0 + boxW - 1, y0 + j, '|', DEAD_FRAME);
    }

    drawCentred(y0 + 2, "Y O U   D I E D", DEAD_TITLE);
    drawCentred(y0 + 4, "The dungeon claims another.", DEAD_TEXT);
    drawCentred(y0 + 6, "SPACE  try again        ESC  quit", DEAD_HINT);

    SDL_RenderPresent(m_sdl);
}

void Game::render()
{
    if (m_state == GameState::Dead) { drawDeathScreen(); return; }

    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);

    for (int row = 0; row < MAP_HEIGHT; ++row)
        for (int col = 0; col < MAP_WIDTH; ++col)
            drawTile(col, row, m_map.tiles[row][col]);

    for (const Monster& m : m_monsters)
        if (m.isAlive() && m_map.tiles[m.y][m.x].visible)
            m_glyphs.drawGlyph(m.x, m.y, m.glyph, m.color);

    m_glyphs.drawGlyph(m_player.x, m_player.y, m_player.glyph, m_player.color);

    SDL_RenderPresent(m_sdl);
}

void Game::run()
{
    render();

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
                SDL_Scancode sc = event.key.scancode;

                if (m_state == GameState::Dead)
                {
                    if (sc == SDL_SCANCODE_SPACE)  { newGame(); dirty = true; }
                    if (sc == SDL_SCANCODE_ESCAPE) running = false;
                    break;
                }

                int dx = 0, dy = 0;
                switch (sc)
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
                    playerAct(dx, dy);
                    dirty = true;
                }
                break;
            }
        }

        if (dirty) render();
    }
}
