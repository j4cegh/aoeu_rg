#include "glyphs.h"
#include "rg.h"

list *glyphs_list;

void glyphs_init()
{
    glyphs_list = list_init();

    glyphs_calc();
}

void glyphs_calc()
{
    SDL_Color col;
	col.r = 255;
	col.g = 255;
	col.b = 255;
	col.a = 255;
    uint16_t ic;
    glEnable(GL_TEXTURE_2D);
    for(ic = ASCII_START; ic < ASCII_END; ++ic)
    {
        SDL_Surface *surf = TTF_RenderGlyph_Blended(rg.font, ic, col);
        if(surf != NULL)
        {
            glyph *add = malloc(sizeof *add);
            add->c = ic;
            add->surf = surf;
            glGenTextures(1, &(add->tex));
            glBindTexture(GL_TEXTURE_2D, add->tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surf->pixels);
            glBindTexture(GL_TEXTURE_2D, 0);
            list_push_back(glyphs_list, add);
        }
    }
    glDisable(GL_TEXTURE_2D);
}

glyph *glyphs_get(uint16_t c)
{
	c = clamp(c, ASCII_START, ASCII_END);
    return list_get_val_at(glyphs_list, c - ASCII_START);
}

void glyph_render_noglsc(glyph *ptr, float x, float y, color4 col)
{
	glBindTexture(GL_TEXTURE_2D, ptr->tex);

    glColor4ub(col.r, col.g, col.b, col.a);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0,0); glVertex2f(x, y);
		glTexCoord2f(1,0); glVertex2f(x + ptr->surf->w, y);
		glTexCoord2f(1,1); glVertex2f(x + ptr->surf->w, y + ptr->surf->h);
		glTexCoord2f(0,1); glVertex2f(x, y + ptr->surf->h);
	}
	glEnd();

}

void glyph_render(glyph *ptr, float x, float y, color4 col)
{
    glEnable(GL_TEXTURE_2D);
	
	glyph_render_noglsc(ptr, x, y, col);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

void glyphs_free()
{
    list_node *n;
    for(n = glyphs_list->start; n != NULL; n = n->next)
    {
        glyph *gl = n->val;
        SDL_FreeSurface(gl->surf);
        if(gl->tex != 0) glDeleteTextures(1, &(gl->tex));
    }
    list_free(glyphs_list, free);
}
