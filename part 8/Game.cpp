#include "Game.h"
#include "FOV.h"
#include <cstdlib>
#include <algorithm>

namespace {
    const SDL_Color BG = { 12, 12, 16, 255 };

    const SDL_Color WALL_LIT  = { 150, 140, 175, 255 };
    const SDL_Color FLOOR_LIT = { 100,  92,  80, 255 };
    const SDL_Color WALL_MEM  = {  58,  54,  72, 255 };
    const SDL_Color FLOOR_MEM = {  40,  37,  33, 255 };

    const SDL_Color POTION_LIT = { 200, 120, 220, 255 };
    const SDL_Color POTION_MEM = {  90,  55, 100, 255 };

    const SDL_Color DEAD_FRAME = { 150,  60,  60, 255 };
    const SDL_Color DEAD_TITLE = { 220,  90,  90, 255 };
    const SDL_Color DEAD_TEXT  = { 210, 200, 205, 255 };
    const SDL_Color DEAD_HINT  = { 120, 112, 112, 255 };

    const SDL_Color UI_FRAME = { 120, 110, 140, 255 };
    const SDL_Color UI_TITLE = { 230, 220, 180, 255 };
    const SDL_Color UI_TEXT  = { 205, 200, 215, 255 };
    const SDL_Color UI_HINT  = { 120, 112, 132, 255 };

    struct MonsterKind { char glyph; SDL_Color color; int hp; int power; const char* name; };
    const MonsterKind KINDS[] = {
        { 'r', { 150, 150, 160, 255 }, 3, 1, "the rat"    },
        { 'k', { 110, 200, 120, 255 }, 5, 2, "the kobold" },
        { 'g', { 120, 170, 230, 255 }, 8, 3, "the goblin" },
    };
    const int NUM_KINDS     = (int)(sizeof(KINDS) / sizeof(KINDS[0]));
    const int MONSTER_COUNT = 8;
    const int ITEM_COUNT    = 6;

    // Pool of appearances; each game assigns a different one to each potion type.
    const char* COLOURS[] = { "red", "blue", "fizzy", "murky", "glowing", "silver" };
    const int   NUM_COLOURS = (int)(sizeof(COLOURS) / sizeof(COLOURS[0]));

    const char* TRUE_NAME[] = { "potion of healing", "potion of poison", "potion of strength" };
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
    m_player.power = 5;
    m_player.alive = true;
    m_state = GameState::Playing;
    FOV::compute(m_map, m_player.x, m_player.y);

    // Assign a random appearance to each potion type; nothing is identified yet.
    int order[NUM_COLOURS];
    for (int i = 0; i < NUM_COLOURS; ++i) order[i] = i;
    std::shuffle(order, order + NUM_COLOURS, m_rng);
    for (int t = 0; t < (int)PotionType::Count; ++t)
    {
        m_appearance[t] = COLOURS[order[t]];
        m_identified[t] = false;
    }

    // Monsters, one per room from room 1 on.
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

    // Scatter potions on random floor tiles inside random rooms.
    m_items.clear();
    m_inventory.clear();
    for (int c = 0; c < ITEM_COUNT; ++c)
    {
        const Room& r = m_map.rooms[std::uniform_int_distribution<int>(0, n - 1)(m_rng)];
        int ix = std::uniform_int_distribution<int>(r.x, r.x + r.w - 1)(m_rng);
        int iy = std::uniform_int_distribution<int>(r.y, r.y + r.h - 1)(m_rng);
        if (m_map.tiles[iy][ix].type != TileType::Floor) continue;
        Item it;
        it.type = (PotionType)std::uniform_int_distribution<int>(0, (int)PotionType::Count - 1)(m_rng);
        it.x = ix;  it.y = iy;
        m_items.push_back(it);
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

std::string Game::potionName(PotionType t) const
{
    if (m_identified[(int)t]) return TRUE_NAME[(int)t];
    return std::string(m_appearance[(int)t]) + " potion";
}

void Game::applyPotion(PotionType t)
{
    bool known = m_identified[(int)t];
    SDL_Log("You quaff the %s.", potionName(t).c_str());

    switch (t)
    {
        case PotionType::Healing:
            m_player.hp = std::min(m_player.maxHp, m_player.hp + 10);
            SDL_Log("A warm glow spreads through you. (HP %d/%d)", m_player.hp, m_player.maxHp);
            break;
        case PotionType::Poison:
            m_player.hp -= 6;
            SDL_Log("You retch — it was poison! (HP %d)", m_player.hp > 0 ? m_player.hp : 0);
            break;
        case PotionType::Strength:
            m_player.power += 1;
            SDL_Log("Your muscles swell. (power %d)", m_player.power);
            break;
        default: break;
    }

    if (!known)
    {
        m_identified[(int)t] = true;          // identify this type for the rest of the run
        SDL_Log("It was %s!", TRUE_NAME[(int)t]);
    }
}

void Game::quaff(int slot)
{
    if (slot < 0 || slot >= (int)m_inventory.size())
        return;                               // no such item — not a turn

    PotionType t = m_inventory[slot];
    applyPotion(t);
    m_inventory.erase(m_inventory.begin() + slot);

    m_state = GameState::Playing;             // quaffing closes the inventory...
    monstersAct();                            // ...and costs a turn
    updateTitle();
    if (m_player.hp <= 0) m_state = GameState::Dead;
}

void Game::pickUpHere()
{
    for (size_t i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].x == m_player.x && m_items[i].y == m_player.y)
        {
            PotionType t = m_items[i].type;
            m_inventory.push_back(t);
            SDL_Log("You pick up a %s.", potionName(t).c_str());
            m_items.erase(m_items.begin() + i);
            break;
        }
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
        pickUpHere();
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
    m_toPlayer.compute(m_map, m_player.x, m_player.y);

    static const int dx[4] = {  0,  0, -1, +1 };
    static const int dy[4] = { -1, +1,  0,  0 };

    for (Monster& m : m_monsters)
    {
        if (!m.isAlive()) continue;
        if (!m_map.tiles[m.y][m.x].visible) { m.wander(m_map, m_rng); continue; }

        if (std::abs(m_player.x - m.x) + std::abs(m_player.y - m.y) == 1)
        {
            attack(m, m_player);
            continue;
        }

        bool flee = (m.hp * 3 <= m.maxHp);
        int bestX = m.x, bestY = m.y;
        int bestVal = m_toPlayer.at(m.x, m.y);

        for (int i = 0; i < 4; ++i)
        {
            int nx = m.x + dx[i], ny = m.y + dy[i];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (m_map.tiles[ny][nx].type != TileType::Floor) continue;
            if (nx == m_player.x && ny == m_player.y) continue;
            if (monsterAt(nx, ny)) continue;

            int v = m_toPlayer.at(nx, ny);
            if (v == DijkstraMap::UNREACHABLE) continue;

            if (flee ? (v > bestVal) : (v < bestVal)) { bestVal = v; bestX = nx; bestY = ny; }
        }

        m.x = bestX; m.y = bestY;
    }
}

void Game::updateTitle()
{
    char buf[112];
    SDL_snprintf(buf, sizeof(buf), "Roguelike - Part 8   |   HP: %d / %d   |   Items: %d   (i = inventory)",
                 m_player.hp, m_player.maxHp, (int)m_inventory.size());
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

void Game::drawInventory()
{
    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);

    const int boxW = 46;
    const int boxH = 4 + (int)m_inventory.size() + (m_inventory.empty() ? 1 : 0);
    const int x0 = (MAP_WIDTH  - boxW) / 2;
    const int y0 = 6;

    for (int i = 0; i < boxW; ++i)
    {
        m_glyphs.drawGlyph(x0 + i, y0,            '=', UI_FRAME);
        m_glyphs.drawGlyph(x0 + i, y0 + boxH - 1, '=', UI_FRAME);
    }
    for (int j = 0; j < boxH; ++j)
    {
        m_glyphs.drawGlyph(x0,            y0 + j, '|', UI_FRAME);
        m_glyphs.drawGlyph(x0 + boxW - 1, y0 + j, '|', UI_FRAME);
    }

    drawCentred(y0 + 1, "I N V E N T O R Y", UI_TITLE);

    if (m_inventory.empty())
    {
        drawCentred(y0 + 3, "(empty - find some potions!)", UI_HINT);
    }
    else
    {
        for (size_t i = 0; i < m_inventory.size(); ++i)
        {
            char line[64];
            SDL_snprintf(line, sizeof(line), "%c)  %s", (char)('a' + i),
                         potionName(m_inventory[i]).c_str());
            // left-aligned a couple of tiles in from the frame
            int col = x0 + 3;
            for (int c = 0; line[c] != '\0'; ++c)
                m_glyphs.drawGlyph(col + c, y0 + 3 + (int)i, line[c], UI_TEXT);
        }
    }

    drawCentred(y0 + boxH - 1 + 2, "letter to quaff      i / ESC  close", UI_HINT);

    SDL_RenderPresent(m_sdl);
}

void Game::render()
{
    if (m_state == GameState::Dead)      { drawDeathScreen(); return; }
    if (m_state == GameState::Inventory) { drawInventory();   return; }

    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);

    for (int row = 0; row < MAP_HEIGHT; ++row)
        for (int col = 0; col < MAP_WIDTH; ++col)
            drawTile(col, row, m_map.tiles[row][col]);

    // Items on the floor — all potions look like '!' (you can't tell type by sight).
    for (const Item& it : m_items)
    {
        const Tile& t = m_map.tiles[it.y][it.x];
        if (t.visible)       m_glyphs.drawGlyph(it.x, it.y, '!', POTION_LIT);
        else if (t.explored) m_glyphs.drawGlyph(it.x, it.y, '!', POTION_MEM);
    }

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

                if (m_state == GameState::Inventory)
                {
                    if (sc == SDL_SCANCODE_I || sc == SDL_SCANCODE_ESCAPE)
                    {
                        m_state = GameState::Playing;
                        dirty = true;
                    }
                    else if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
                    {
                        quaff(sc - SDL_SCANCODE_A);   // may close & take a turn
                        dirty = true;
                    }
                    break;
                }

                // --- Playing ---
                if (sc == SDL_SCANCODE_I)
                {
                    m_state = GameState::Inventory;   // opening costs no turn
                    dirty = true;
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
