#include "text.h"
#include "rg.h"
#include "utils.h"

text *text_init(char *str, float x, float y, color4 color)
{
	text *ret = malloc(sizeof *ret);
	ret->str = dupe_str(str);
	ret->pos.x = x;
	ret->pos.y = y;
	ret->draw_bg = 0;
	ret->surf = NULL;
	ret->fon_tex = 0;
	ret->col = color;
	ret->bg_col = color4_gen_alpha(0, 0, 0, 220);
	ret->enable_smoothing = 1;
	ret->vertical_offset = -3;
	text_set_position(ret, x, y);
	text_calc_bounds(ret);
	return ret;
}

void text_render(text *ptr)
{
	if(ptr->surf == NULL || ptr->fon_tex == 0) text_gen_surface(ptr);

	if(ptr->surf == NULL || ptr->fon_tex == 0) return;
	
	if(ptr->draw_bg)
	{
		glColor4ub(color4_2func_alpha(ptr->bg_col));
		glBegin(GL_QUADS);
		glVertex2f(ptr->pos.x, ptr->pos.y);
		glVertex2f(ptr->pos.x + ptr->surf->w, ptr->pos.y);
		glVertex2f(ptr->pos.x + ptr->surf->w, ptr->pos.y + ptr->surf->h);
		glVertex2f(ptr->pos.x, ptr->pos.y + ptr->surf->h);
		glEnd();
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ptr->fon_tex);
	
	glColor4ub(ptr->col.r, ptr->col.g, ptr->col.b, ptr->col.a);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0,0); glVertex2f(ptr->pos.x, ptr->pos.y);
		glTexCoord2f(1,0); glVertex2f(ptr->pos.x + ptr->surf->w, ptr->pos.y);
		glTexCoord2f(1,1); glVertex2f(ptr->pos.x + ptr->surf->w, ptr->pos.y + ptr->surf->h);
		glTexCoord2f(0,1); glVertex2f(ptr->pos.x, ptr->pos.y + ptr->surf->h);
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

void text_gen_surface(text *ptr)
{
	if(!rg.font) return;
	
	if(ptr->fon_tex != 0) glDeleteTextures(1, &(ptr->fon_tex));
	if(ptr->surf != NULL) SDL_FreeSurface(ptr->surf);

	SDL_Color col;
	col.r = 255;
	col.g = 255;
	col.b = 255;
	col.a = 255;
	ptr->surf = TTF_RenderText_Blended(rg.font, ptr->str, col);

	glGenTextures(1, &(ptr->fon_tex));

	if(ptr->surf == NULL || ptr->fon_tex == 0) return;

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, ptr->fon_tex);

	if(ptr->enable_smoothing)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ptr->surf->w, ptr->surf->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, ptr->surf->pixels);

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
}

void text_calc_bounds(text *ptr)
{
	if(rg.font) TTF_SizeText(rg.font, ptr->str, &(ptr->size.x), &(ptr->size.y));
}

void text_set_position(text *ptr, float x, float y)
{
	ptr->pos.x = x;
	ptr->pos.y = y + ptr->vertical_offset;
}

void text_set_string(text *ptr, char *new_str)
{
	if(strs_are_equal(ptr->str, new_str)) return;
	if(ptr->str) free(ptr->str);
	ptr->str = dupe_str(new_str);
	text_calc_bounds(ptr);
	text_gen_surface(ptr);
}

void text_set_color(text *ptr, color4 color)
{
	ptr->col = color;
}

void text_set_opacity(text *ptr, float val)
{
	float rval = scale_value_to(clamp(val, 0.0f, 1.0f), 0.0f, 1.0f, 0, 255);
	ptr->col.a = rval;
	ptr->bg_col.a = rval;
}

void text_free(text *ptr)
{
	glDeleteTextures(1, &(ptr->fon_tex));
	SDL_FreeSurface(ptr->surf);
	free(ptr->str);
	free(ptr);
}
