#include "Game.h"
#include "FOV.h"
#include <cstdlib>
#include <algorithm>

namespace {
    const SDL_Color BG = { 12, 12, 16, 255 };

    const SDL_Color WALL_LIT   = { 150, 140, 175, 255 };
    const SDL_Color FLOOR_LIT  = { 100,  92,  80, 255 };
    const SDL_Color WALL_MEM   = {  58,  54,  72, 255 };
    const SDL_Color FLOOR_MEM  = {  40,  37,  33, 255 };
    const SDL_Color ITEM_MEM   = {  72,  66,  80, 255 };
    const SDL_Color STAIRS_LIT = { 235, 215,  90, 255 };
    const SDL_Color STAIRS_MEM = {  95,  88,  48, 255 };

    const SDL_Color DEAD_FRAME = { 150,  60,  60, 255 };
    const SDL_Color DEAD_TITLE = { 220,  90,  90, 255 };
    const SDL_Color DEAD_TEXT  = { 210, 200, 205, 255 };
    const SDL_Color DEAD_HINT  = { 120, 112, 112, 255 };

    const SDL_Color UI_TITLE = { 230, 220, 180, 255 };
    const SDL_Color UI_TEXT  = { 205, 200, 215, 255 };
    const SDL_Color UI_GEAR  = { 150, 205, 160, 255 };
    const SDL_Color UI_HINT  = { 120, 112, 132, 255 };
    const SDL_Color MENU_HI  = { 255, 230, 150, 255 };

    const char* COLOURS[] = { "red", "blue", "fizzy", "murky", "glowing", "silver" };
    const int   NUM_COLOURS = (int)(sizeof(COLOURS) / sizeof(COLOURS[0]));
    const char* POTION_TRUE[] = {
        "potion of healing", "potion of poison", "potion of strength", "potion of regeneration"
    };
    const char* LABELS[] = { "ZELGO MER", "XYZZY", "FOOBIE BLETCH", "ELBIB YLOH", "VENZAR" };
    const int   NUM_LABELS = (int)(sizeof(LABELS) / sizeof(LABELS[0]));
    const char* SCROLL_TRUE[] = { "scroll of lightning", "scroll of fireball" };

    const int BASE_DEFENSE = 0;
    const int POISON_TURNS = 6, REGEN_TURNS = 12, VENOM_TURNS = 5;
    const int LIGHTNING_DMG = 6, FIREBALL_DMG = 5;
}

Game::Game(SDL_Window* window, SDL_Renderer* sdl, GlyphCache& glyphs)
    : m_window(window), m_sdl(sdl), m_glyphs(glyphs)
{
    m_state = GameState::ClassSelect;   // start at the hero-choice screen
}

Item Game::makeWeapon(const char* name, int bonus)
{ Item it; it.category = ItemCategory::Weapon; it.name = name; it.bonus = bonus; it.glyph = '/'; it.color = { 185,185,205,255 }; return it; }
Item Game::makeArmour(const char* name, int bonus)
{ Item it; it.category = ItemCategory::Armour; it.name = name; it.bonus = bonus; it.glyph = '['; it.color = { 200,160,110,255 }; return it; }
Item Game::makeScroll(ScrollType s)
{ Item it; it.category = ItemCategory::Scroll; it.scroll = s; it.glyph = '?'; it.color = { 120,200,220,255 }; return it; }
Item Game::makePotion(PotionType p)
{ Item it; it.category = ItemCategory::Potion; it.potion = p; it.glyph = '!'; it.color = { 200,120,220,255 }; return it; }

void Game::recomputeStats()
{
    m_player.power   = m_basePower  + m_strengthBonus + (m_hasWeapon ? m_weapon.bonus : 0);
    m_player.defense = BASE_DEFENSE +                   (m_hasArmour ? m_armour.bonus : 0);
}

void Game::newGame(PlayerClass cls)
{
    m_class = cls;
    m_levels.clear();
    m_depth = 0;
    m_levels.emplace_back();
    m_levels[0].generate(0, m_rng);
    Level& L = cur();

    m_player = Player{};
    m_strengthBonus = 0;
    m_magicBonus    = 0;
    m_hasWeapon = false;
    m_hasArmour = false;
    m_inventory.clear();

    switch (cls)
    {
        case PlayerClass::Fighter:
            m_baseMaxHp = 36;  m_basePower = 6;
            m_weapon = makeWeapon("sword", 4);      m_hasWeapon = true;
            m_armour = makeArmour("chain mail", 3); m_hasArmour = true;
            m_inventory.push_back(makePotion(PotionType::Healing));
            break;
        case PlayerClass::Rogue:
            m_baseMaxHp = 28;  m_basePower = 5;
            m_weapon = makeWeapon("dagger", 2);  m_hasWeapon = true;
            m_armour = makeArmour("leather", 1); m_hasArmour = true;
            m_inventory.push_back(makePotion(PotionType::Healing));
            m_inventory.push_back(makeScroll(ScrollType::Lightning));
            break;
        case PlayerClass::Mage:
            m_baseMaxHp = 22;  m_basePower = 3;  m_magicBonus = 6;
            m_inventory.push_back(makeScroll(ScrollType::Lightning));
            m_inventory.push_back(makeScroll(ScrollType::Fireball));
            m_inventory.push_back(makePotion(PotionType::Healing));
            break;
    }

    m_player.maxHp = m_baseMaxHp;
    m_player.hp    = m_baseMaxHp;
    recomputeStats();

    m_player.x = L.map.rooms[0].centreX();
    m_player.y = L.map.rooms[0].centreY();
    m_state = GameState::Playing;

    {
        int order[NUM_COLOURS];
        for (int i = 0; i < NUM_COLOURS; ++i) order[i] = i;
        std::shuffle(order, order + NUM_COLOURS, m_rng);
        for (int t = 0; t < (int)PotionType::Count; ++t) { m_potionAppear[t] = COLOURS[order[t]]; m_potionIdent[t] = false; }
    }
    {
        int order[NUM_LABELS];
        for (int i = 0; i < NUM_LABELS; ++i) order[i] = i;
        std::shuffle(order, order + NUM_LABELS, m_rng);
        for (int t = 0; t < (int)ScrollType::Count; ++t) { m_scrollAppear[t] = LABELS[order[t]]; m_scrollIdent[t] = false; }
    }

    FOV::compute(L.map, m_player.x, m_player.y);
    updateTitle();
}

void Game::descend()
{
    Level& here = cur();
    if (m_player.x != here.stairsX || m_player.y != here.stairsY) { SDL_Log("There are no stairs here."); return; }
    m_depth++;
    if (m_depth >= (int)m_levels.size()) { m_levels.emplace_back(); m_levels.back().generate(m_depth, m_rng); }
    Level& L = cur();
    m_player.x = L.map.rooms[0].centreX();
    m_player.y = L.map.rooms[0].centreY();
    FOV::compute(L.map, m_player.x, m_player.y);
    SDL_Log("You descend to depth %d.", m_depth + 1);
    updateTitle();
}

Monster* Game::monsterAt(int x, int y)
{
    for (Monster& m : cur().monsters)
        if (m.isAlive() && m.x == x && m.y == y) return &m;
    return nullptr;
}

Monster* Game::nearestVisibleMonster()
{
    Level& L = cur();
    Monster* best = nullptr; int bestDist = 1 << 30;
    for (Monster& m : L.monsters)
    {
        if (!m.isAlive() || !L.map.tiles[m.y][m.x].visible) continue;
        int d = std::abs(m.x - m_player.x) + std::abs(m.y - m_player.y);
        if (d < bestDist) { bestDist = d; best = &m; }
    }
    return best;
}

void Game::attack(Entity& atk, Entity& def)
{
    int dmg = atk.power - def.defense;
    if (dmg < 1) dmg = 1;
    def.hp -= dmg;
    SDL_Log("%s hits %s for %d. (%s: %d hp)", atk.name, def.name, dmg, def.name, def.hp > 0 ? def.hp : 0);
    if (def.hp <= 0) { def.hp = 0; def.alive = false; SDL_Log("%s dies.", def.name); }
}

void Game::tickStatuses(Entity& e)
{
    if (e.statusTurns[(int)Status::Poisoned] > 0)
    {
        e.hp -= 1; e.statusTurns[(int)Status::Poisoned]--;
        SDL_Log("%s is wracked by poison. (hp %d)", e.name, e.hp > 0 ? e.hp : 0);
        if (e.hp <= 0) { e.hp = 0; e.alive = false; SDL_Log("%s succumbs to poison.", e.name); }
    }
    if (e.statusTurns[(int)Status::Regen] > 0)
    {
        if (e.hp < e.maxHp) e.hp += 1;
        e.statusTurns[(int)Status::Regen]--;
    }
}

std::string Game::potionName(PotionType t) const
{
    if (m_potionIdent[(int)t]) return POTION_TRUE[(int)t];
    return std::string(m_potionAppear[(int)t]) + " potion";
}
std::string Game::scrollName(ScrollType t) const
{
    if (m_scrollIdent[(int)t]) return SCROLL_TRUE[(int)t];
    return std::string("scroll labelled \"") + m_scrollAppear[(int)t] + "\"";
}
std::string Game::itemLabel(const Item& it) const
{
    if (it.category == ItemCategory::Potion) return potionName(it.potion);
    if (it.category == ItemCategory::Scroll) return scrollName(it.scroll);
    char buf[48];
    SDL_snprintf(buf, sizeof(buf), "%s (+%d %s)", it.name, it.bonus, it.category == ItemCategory::Weapon ? "pow" : "def");
    return buf;
}

void Game::quaff(PotionType t)
{
    bool known = m_potionIdent[(int)t];
    SDL_Log("You quaff the %s.", potionName(t).c_str());
    switch (t)
    {
        case PotionType::Healing:  m_player.hp = std::min(m_player.maxHp, m_player.hp + 10); SDL_Log("A warm glow spreads through you. (HP %d/%d)", m_player.hp, m_player.maxHp); break;
        case PotionType::Poison:   m_player.addStatus(Status::Poisoned, POISON_TURNS); SDL_Log("Your veins burn — poison!"); break;
        case PotionType::Strength: m_strengthBonus += 1; recomputeStats(); SDL_Log("Your muscles swell. (power %d)", m_player.power); break;
        case PotionType::Regen:    m_player.addStatus(Status::Regen, REGEN_TURNS); SDL_Log("Your wounds begin to knit closed."); break;
        default: break;
    }
    if (!known) { m_potionIdent[(int)t] = true; SDL_Log("It was %s!", POTION_TRUE[(int)t]); }
}

void Game::readScroll(ScrollType t)
{
    bool known = m_scrollIdent[(int)t];
    SDL_Log("You read the %s.", scrollName(t).c_str());
    Monster* target = nearestVisibleMonster();
    auto zap = [&](Monster& m, int dmg)
    {
        m.hp -= dmg;
        SDL_Log("%s is blasted for %d. (%s: %d hp)", m.name, dmg, m.name, m.hp > 0 ? m.hp : 0);
        if (m.hp <= 0) { m.hp = 0; m.alive = false; SDL_Log("%s is destroyed.", m.name); }
    };
    if (!target) SDL_Log("The magic crackles and fades — nothing in sight.");
    else if (t == ScrollType::Lightning) zap(*target, LIGHTNING_DMG + m_magicBonus);
    else
    {
        int tx = target->x, ty = target->y;
        for (Monster& m : cur().monsters)
            if (m.isAlive() && std::abs(m.x - tx) <= 1 && std::abs(m.y - ty) <= 1)
                zap(m, FIREBALL_DMG + m_magicBonus);
    }
    if (!known) { m_scrollIdent[(int)t] = true; SDL_Log("It was a %s!", SCROLL_TRUE[(int)t]); }
}

void Game::equip(const Item& it)
{
    if (it.category == ItemCategory::Weapon)
    {
        if (m_hasWeapon) m_inventory.push_back(m_weapon);
        m_weapon = it; m_hasWeapon = true; SDL_Log("You wield the %s.", it.name);
    }
    else
    {
        if (m_hasArmour) m_inventory.push_back(m_armour);
        m_armour = it; m_hasArmour = true; SDL_Log("You don the %s.", it.name);
    }
    recomputeStats();
}

void Game::useItem(int slot)
{
    if (slot < 0 || slot >= (int)m_inventory.size()) return;
    Item it = m_inventory[slot];
    m_inventory.erase(m_inventory.begin() + slot);
    switch (it.category)
    {
        case ItemCategory::Potion: quaff(it.potion);      break;
        case ItemCategory::Scroll: readScroll(it.scroll); break;
        default:                   equip(it);             break;
    }
    m_state = GameState::Playing;
    endPlayerTurn();
}

void Game::pickUpHere()
{
    Level& L = cur();
    for (size_t i = 0; i < L.items.size(); ++i)
        if (L.items[i].x == m_player.x && L.items[i].y == m_player.y)
        {
            m_inventory.push_back(L.items[i]);
            SDL_Log("You pick up a %s.", itemLabel(L.items[i]).c_str());
            L.items.erase(L.items.begin() + i);
            break;
        }
}

void Game::playerAct(int dx, int dy)
{
    bool tookTurn = false;
    Monster* target = monsterAt(m_player.x + dx, m_player.y + dy);
    if (target) { attack(m_player, *target); tookTurn = true; }
    else if (m_player.tryMove(dx, dy, cur().map)) { FOV::compute(cur().map, m_player.x, m_player.y); pickUpHere(); tookTurn = true; }
    if (tookTurn) endPlayerTurn();
}

void Game::endPlayerTurn()
{
    monstersAct();
    tickStatuses(m_player);
    updateTitle();
    if (m_player.hp <= 0) m_state = GameState::Dead;
}

void Game::monstersAct()
{
    Level& L = cur();
    m_toPlayer.compute(L.map, m_player.x, m_player.y);
    static const int dx[4] = { 0, 0, -1, +1 };
    static const int dy[4] = { -1, +1, 0, 0 };

    for (Monster& m : L.monsters)
    {
        if (!m.isAlive()) continue;
        tickStatuses(m);
        if (!m.isAlive()) continue;
        if (!L.map.tiles[m.y][m.x].visible) { m.wander(L.map, m_rng); continue; }

        if (std::abs(m_player.x - m.x) + std::abs(m_player.y - m.y) == 1)
        {
            attack(m, m_player);
            if (m.venomous && m_player.isAlive()) { m_player.addStatus(Status::Poisoned, VENOM_TURNS); SDL_Log("%s's bite is venomous!", m.name); }
            continue;
        }

        bool flee = (m.hp * 3 <= m.maxHp);
        int bestX = m.x, bestY = m.y, bestVal = m_toPlayer.at(m.x, m.y);
        for (int i = 0; i < 4; ++i)
        {
            int nx = m.x + dx[i], ny = m.y + dy[i];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (L.map.tiles[ny][nx].type != TileType::Floor) continue;
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
    const char* cn = (m_class == PlayerClass::Fighter) ? "Fighter"
                   : (m_class == PlayerClass::Rogue)   ? "Rogue" : "Mage";
    std::string status;
    if (m_player.statusTurns[(int)Status::Poisoned] > 0) status += "  POISON(" + std::to_string(m_player.statusTurns[(int)Status::Poisoned]) + ")";
    if (m_player.statusTurns[(int)Status::Regen]    > 0) status += "  REGEN("  + std::to_string(m_player.statusTurns[(int)Status::Regen])    + ")";

    char buf[208];
    SDL_snprintf(buf, sizeof(buf),
        "Roguelike - Part 14   |   %s   Depth %d   HP %d/%d POW%d DEF%d%s   (i inv, > stairs)",
        cn, m_depth + 1, m_player.hp, m_player.maxHp, m_player.power, m_player.defense, status.c_str());
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
    int len = (int)SDL_strlen(text), startCol = (MAP_WIDTH - len) / 2;
    for (int i = 0; i < len; ++i) m_glyphs.drawGlyph(startCol + i, row, text[i], color);
}
void Game::drawLeft(int col, int row, const char* text, SDL_Color color)
{
    for (int i = 0; text[i] != '\0'; ++i) m_glyphs.drawGlyph(col + i, row, text[i], color);
}

void Game::drawClassSelect()
{
    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);
    drawCentred(8,  "C H O O S E   Y O U R   H E R O", MENU_HI);
    drawCentred(13, "1)  Fighter  -  tough; starts with sword & chain mail", UI_TEXT);
    drawCentred(15, "2)  Rogue    -  nimble; dagger, leather & a scroll",    UI_TEXT);
    drawCentred(17, "3)  Mage     -  frail, but devastating magic; 2 scrolls", UI_TEXT);
    drawCentred(22, "press 1, 2 or 3        ESC  quit", UI_HINT);
    SDL_RenderPresent(m_sdl);
}

void Game::drawDeathScreen()
{
    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);
    const int boxW = 40, boxH = 9, x0 = (MAP_WIDTH - boxW) / 2, y0 = (MAP_HEIGHT - boxH) / 2;
    for (int i = 0; i < boxW; ++i) { m_glyphs.drawGlyph(x0 + i, y0, '=', DEAD_FRAME); m_glyphs.drawGlyph(x0 + i, y0 + boxH - 1, '=', DEAD_FRAME); }
    for (int j = 0; j < boxH; ++j) { m_glyphs.drawGlyph(x0, y0 + j, '|', DEAD_FRAME); m_glyphs.drawGlyph(x0 + boxW - 1, y0 + j, '|', DEAD_FRAME); }
    drawCentred(y0 + 2, "Y O U   D I E D", DEAD_TITLE);
    drawCentred(y0 + 4, "The dungeon claims another.", DEAD_TEXT);
    drawCentred(y0 + 6, "SPACE  choose a new hero        ESC  quit", DEAD_HINT);
    SDL_RenderPresent(m_sdl);
}

void Game::drawInventory()
{
    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);
    const int x0 = 8, y0 = 4, colText = x0 + 3;
    drawLeft(x0, y0, "== INVENTORY ==", UI_TITLE);
    char gear[64];
    SDL_snprintf(gear, sizeof(gear), "Wielding: %s", m_hasWeapon ? itemLabel(m_weapon).c_str() : "(bare fists)");
    drawLeft(x0, y0 + 2, gear, UI_GEAR);
    SDL_snprintf(gear, sizeof(gear), "Wearing:  %s", m_hasArmour ? itemLabel(m_armour).c_str() : "(no armour)");
    drawLeft(x0, y0 + 3, gear, UI_GEAR);
    drawLeft(x0, y0 + 5, "Pack:", UI_TITLE);
    if (m_inventory.empty()) drawLeft(colText, y0 + 6, "(empty)", UI_HINT);
    else for (size_t i = 0; i < m_inventory.size(); ++i)
    {
        char line[64];
        SDL_snprintf(line, sizeof(line), "%c)  %s", (char)('a' + i), itemLabel(m_inventory[i]).c_str());
        drawLeft(colText, y0 + 6 + (int)i, line, UI_TEXT);
    }
    drawLeft(x0, MAP_HEIGHT - 3, "letter to use/read/equip      i / ESC  close", UI_HINT);
    SDL_RenderPresent(m_sdl);
}

void Game::render()
{
    if (m_state == GameState::ClassSelect) { drawClassSelect(); return; }
    if (m_state == GameState::Dead)        { drawDeathScreen(); return; }
    if (m_state == GameState::Inventory)   { drawInventory();   return; }

    Level& L = cur();
    SDL_SetRenderDrawColor(m_sdl, BG.r, BG.g, BG.b, BG.a);
    SDL_RenderClear(m_sdl);

    for (int row = 0; row < MAP_HEIGHT; ++row)
        for (int col = 0; col < MAP_WIDTH; ++col)
            drawTile(col, row, L.map.tiles[row][col]);

    {
        const Tile& t = L.map.tiles[L.stairsY][L.stairsX];
        if (t.visible)       m_glyphs.drawGlyph(L.stairsX, L.stairsY, '>', STAIRS_LIT);
        else if (t.explored) m_glyphs.drawGlyph(L.stairsX, L.stairsY, '>', STAIRS_MEM);
    }
    for (const Item& it : L.items)
    {
        const Tile& t = L.map.tiles[it.y][it.x];
        if (t.visible)       m_glyphs.drawGlyph(it.x, it.y, it.glyph, it.color);
        else if (t.explored) m_glyphs.drawGlyph(it.x, it.y, it.glyph, ITEM_MEM);
    }
    for (const Monster& m : L.monsters)
        if (m.isAlive() && L.map.tiles[m.y][m.x].visible)
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
            case SDL_EVENT_QUIT: running = false; break;
            case SDL_EVENT_WINDOW_EXPOSED: dirty = true; break;
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_Scancode sc = event.key.scancode;

                if (m_state == GameState::ClassSelect)
                {
                    if (sc == SDL_SCANCODE_1) { newGame(PlayerClass::Fighter); dirty = true; }
                    if (sc == SDL_SCANCODE_2) { newGame(PlayerClass::Rogue);   dirty = true; }
                    if (sc == SDL_SCANCODE_3) { newGame(PlayerClass::Mage);    dirty = true; }
                    if (sc == SDL_SCANCODE_ESCAPE) running = false;
                    break;
                }
                if (m_state == GameState::Dead)
                {
                    if (sc == SDL_SCANCODE_SPACE)  { m_state = GameState::ClassSelect; dirty = true; }
                    if (sc == SDL_SCANCODE_ESCAPE) running = false;
                    break;
                }
                if (m_state == GameState::Inventory)
                {
                    if (sc == SDL_SCANCODE_I || sc == SDL_SCANCODE_ESCAPE) { m_state = GameState::Playing; dirty = true; }
                    else if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) { useItem(sc - SDL_SCANCODE_A); dirty = true; }
                    break;
                }
                if (sc == SDL_SCANCODE_I)      { m_state = GameState::Inventory; dirty = true; break; }
                if (sc == SDL_SCANCODE_PERIOD) { descend(); dirty = true; break; }

                int dx = 0, dy = 0;
                switch (sc)
                {
                    case SDL_SCANCODE_UP:    case SDL_SCANCODE_W: dy = -1; break;
                    case SDL_SCANCODE_DOWN:  case SDL_SCANCODE_S: dy = +1; break;
                    case SDL_SCANCODE_LEFT:  case SDL_SCANCODE_A: dx = -1; break;
                    case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D: dx = +1; break;
                    case SDL_SCANCODE_ESCAPE: running = false; break;
                    default: break;
                }
                if (dx != 0 || dy != 0) { playerAct(dx, dy); dirty = true; }
                break;
            }
        }
        if (dirty) render();
    }
}
