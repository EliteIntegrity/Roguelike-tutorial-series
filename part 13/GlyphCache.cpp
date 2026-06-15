#include "GlyphCache.h"
#include "Map.h"                 // for TILE_SIZE
#include <SDL3_ttf/SDL_ttf.h>

GlyphCache::GlyphCache(SDL_Renderer* sdl, const char* fontPath, float ptSize)
    : m_sdl(sdl)
{
    TTF_Font* font = TTF_OpenFont(fontPath, ptSize);
    if (!font)
    {
        SDL_Log("GlyphCache: could not open font '%s': %s", fontPath, SDL_GetError());
        return;
    }

    const SDL_Color white = { 255, 255, 255, 255 };

    for (int c = FIRST_CHAR; c <= LAST_CHAR; ++c)
    {
        char str[2] = { (char)c, '\0' };

        // Render one white glyph onto a fresh transparent surface, then turn
        // it straight into a texture. Creating the texture DIRECTLY from this
        // surface (with no intermediate blit into an atlas) is what preserves
        // the glyph's alpha correctly — the bug that made the old atlas
        // approach render near-invisible.
        SDL_Surface* surf = TTF_RenderText_Blended(font, str, 0, white);
        if (!surf) continue;

        Glyph& g = m_glyphs[c - FIRST_CHAR];
        g.w   = surf->w;
        g.h   = surf->h;
        g.tex = SDL_CreateTextureFromSurface(m_sdl, surf);
        if (g.tex)
            SDL_SetTextureBlendMode(g.tex, SDL_BLENDMODE_BLEND);

        SDL_DestroySurface(surf);
    }

    TTF_CloseFont(font);
    m_loaded = true;
}

GlyphCache::~GlyphCache()
{
    for (Glyph& g : m_glyphs)
        if (g.tex) SDL_DestroyTexture(g.tex);
}

void GlyphCache::drawGlyph(int col, int row, char ch, SDL_Color color) const
{
    if (ch < FIRST_CHAR || ch > LAST_CHAR) ch = '?';

    const Glyph& g = m_glyphs[ch - FIRST_CHAR];
    if (!g.tex) return;

    SDL_SetTextureColorMod(g.tex, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(g.tex, color.a);

    // Centre the glyph in its cell at natural size. The +0.5 rounding keeps
    // the glyph on whole pixels, which keeps the edges sharp.
    float x = col * TILE_SIZE + (TILE_SIZE - g.w) * 0.5f;
    float y = row * TILE_SIZE + (TILE_SIZE - g.h) * 0.5f;
    SDL_FRect dst = { SDL_floorf(x + 0.5f), SDL_floorf(y + 0.5f),
                      (float)g.w,           (float)g.h };

    SDL_RenderTexture(m_sdl, g.tex, nullptr, &dst);
}
