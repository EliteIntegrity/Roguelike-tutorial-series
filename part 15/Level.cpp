#include "Level.h"

namespace {
    struct MonsterKind { char glyph; SDL_Color color; int hp; int power; const char* name; bool venom; };
    const MonsterKind KINDS[] = {
        { 'r', { 150, 150, 160, 255 }, 3, 2, "the rat",    false },
        { 'k', { 110, 200, 120, 255 }, 5, 4, "the kobold", false },
        { 'g', { 120, 170, 230, 255 }, 8, 6, "the goblin", false },
        { 'S', {  90, 200,  90, 255 }, 4, 3, "the snake",  true  },
    };
    const int NUM_KINDS = (int)(sizeof(KINDS) / sizeof(KINDS[0]));

    Item randomItem(std::mt19937& rng)
    {
        Item it;
        int roll = std::uniform_int_distribution<int>(0, 99)(rng);
        if (roll < 50)
        {
            it.category = ItemCategory::Potion;
            it.potion   = (PotionType)std::uniform_int_distribution<int>(0, (int)PotionType::Count - 1)(rng);
            it.glyph = '!';  it.color = { 200, 120, 220, 255 };
        }
        else if (roll < 70)
        {
            it.category = ItemCategory::Scroll;
            it.scroll   = (ScrollType)std::uniform_int_distribution<int>(0, (int)ScrollType::Count - 1)(rng);
            it.glyph = '?';  it.color = { 120, 200, 220, 255 };
        }
        else if (roll < 85)
        {
            it.category = ItemCategory::Weapon;
            it.glyph = '/';  it.color = { 185, 185, 205, 255 };
            if (std::uniform_int_distribution<int>(0, 1)(rng)) { it.name = "sword";  it.bonus = 4; }
            else                                               { it.name = "dagger"; it.bonus = 2; }
        }
        else
        {
            it.category = ItemCategory::Armour;
            it.glyph = '[';  it.color = { 200, 160, 110, 255 };
            if (std::uniform_int_distribution<int>(0, 1)(rng)) { it.name = "chain mail"; it.bonus = 3; }
            else                                               { it.name = "leather";    it.bonus = 1; }
        }
        return it;
    }
}

void Level::generate(int depth, std::mt19937& rng)
{
    if (depth % 2 == 1) map.generateCaves();
    else                map.generate();

    monsters.clear();
    items.clear();

    int n = (int)map.rooms.size();

    const Room& last = map.rooms[n - 1];
    stairsX = last.centreX();
    stairsY = last.centreY();

    int monsterCount = 8 + depth * 2;
    for (int i = 1; i < n && (int)monsters.size() < monsterCount; ++i)
    {
        const MonsterKind& k = KINDS[std::uniform_int_distribution<int>(0, NUM_KINDS - 1)(rng)];
        Monster m;
        m.x = map.rooms[i].centreX();
        m.y = map.rooms[i].centreY();
        m.glyph = k.glyph;  m.color = k.color;
        m.hp = m.maxHp = k.hp + depth;
        m.power = k.power + depth / 2;
        m.name = k.name;
        m.venomous = k.venom;
        monsters.push_back(m);
    }

    int itemCount = 7;
    for (int c = 0; c < itemCount; ++c)
    {
        const Room& r = map.rooms[std::uniform_int_distribution<int>(0, n - 1)(rng)];
        int ix = std::uniform_int_distribution<int>(r.x, r.x + r.w - 1)(rng);
        int iy = std::uniform_int_distribution<int>(r.y, r.y + r.h - 1)(rng);
        if (map.tiles[iy][ix].type != TileType::Floor) continue;
        Item it = randomItem(rng);
        it.x = ix;  it.y = iy;
        items.push_back(it);
    }
}
