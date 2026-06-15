#pragma once
#include <SDL3/SDL.h>

// ---------------------------------------------------------------------------
//  GlyphCache — the ASCII renderer used by every part of this series.
//
//  At construction it renders each printable ASCII character once, in white,
//  into its own small SDL_Texture. At draw time it tints that texture with
//  SDL_SetTextureColorMod and blits it CENTRED in a TILE_SIZE cell AT ITS
//  NATURAL SIZE — never stretched. Drawing at natural size is what keeps the
//  glyphs crisp; stretching a 12x16 glyph to fill a 20x20 cell is what
//  deforms them.
// ---------------------------------------------------------------------------
class GlyphCache
{
public:
    GlyphCache(SDL_Renderer* sdl, const char* fontPath, float ptSize);
    ~GlyphCache();

    // Was the font loaded and the cache built successfully?
    bool ok() const { return m_loaded; }

    // Draw character `ch`, centred in grid cell (col, row), tinted `color`.
    void drawGlyph(int col, int row, char ch, SDL_Color color) const;

private:
    SDL_Renderer* m_sdl    = nullptr;
    bool          m_loaded = false;

    static constexpr int FIRST_CHAR = 32;    // space
    static constexpr int LAST_CHAR  = 126;   // tilde
    static constexpr int NUM_CHARS  = LAST_CHAR - FIRST_CHAR + 1;

    struct Glyph
    {
        SDL_Texture* tex = nullptr;
        int          w   = 0;     // natural rendered width
        int          h   = 0;     // natural rendered height
    };
    Glyph m_glyphs[NUM_CHARS];
};
