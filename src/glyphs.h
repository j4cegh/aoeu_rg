#pragma once

#include "includes.h"
#include "list.h"

typedef struct glyph
{
    uint16_t c;
    SDL_Surface *surf;
    GLuint tex;
} glyph;

void glyphs_init();
void glyphs_calc();
glyph *glyphs_get(uint16_t c);
void glyph_render_noglsc(glyph *ptr, float x, float y, color4 col);
void glyph_render(glyph *ptr, float x, float y, color4 col);
void glyphs_free();

extern list *glyphs_list;

