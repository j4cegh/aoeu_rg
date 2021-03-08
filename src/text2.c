#include "text2.h"
#include "rg.h"
#include "utils.h"

text2 *text2_init(char *str, float x, float y, color4 color)
{
	text2 *ret = (text2*) malloc(sizeof *ret);
	ret->str = dupe_str(str);
	ret->pos.x = x;
	ret->pos.y = y;
	ret->draw_bg = 0;
	ret->surfs = list_init();
	ret->fon_tex = 0;
	ret->col = color;
	ret->bg_col = color4_gen_alpha(0, 0, 0, 220);
	ret->enable_smoothing = 0;
	text2_set_position(ret, x, y);
	return ret;
}

void text2_render(text2 *ptr)
{
	if(ptr->surfs->count < 1 || ptr->fon_tex == 0)
	{
		text2_gen_surfaces(ptr);
		return;
	}
	
	if(ptr->draw_bg)
	{
		glColor4ub(color4_2func_alpha(ptr->bg_col));
		glBegin(GL_QUADS);
		glVertex2f(ptr->pos.x, ptr->pos.y);
		glVertex2f(ptr->pos.x + ptr->size.x, ptr->pos.y);
		glVertex2f(ptr->pos.x + ptr->size.x, ptr->pos.y + ptr->size.y);
		glVertex2f(ptr->pos.x, ptr->pos.y + ptr->size.y);
		glEnd();
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ptr->fon_tex);
	
	glColor4ub(ptr->col.r, ptr->col.g, ptr->col.b, ptr->col.a);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0,0); glVertex2f(ptr->pos.x, ptr->pos.y);
		glTexCoord2f(1,0); glVertex2f(ptr->pos.x + ptr->size.x, ptr->pos.y);
		glTexCoord2f(1,1); glVertex2f(ptr->pos.x + ptr->size.x, ptr->pos.y + ptr->size.y);
		glTexCoord2f(0,1); glVertex2f(ptr->pos.x, ptr->pos.y + ptr->size.y);
	}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

void text2_gen_surfaces(text2 *ptr)
{
	if(!rg.font) return;
	
	if(ptr->fon_tex != 0) glDeleteTextures(1, &(ptr->fon_tex));
	
	text2_clear_surfs(ptr, 0);
	
	list *lines = split_str(ptr->str, "\n");

	list_node *n;
	for(n=lines->start;n!=NULL; n=n->next)
	{
		char *s = n->val;
		text2_add_surf(ptr, s, ptr->col);
	}

	list_free(lines, free);

	glGenTextures(1, &(ptr->fon_tex));

	if(ptr->surfs->count < 1 || ptr->fon_tex == 0) return;

	v2i combined = { 0, 0 };

	
	for(n = ptr->surfs->start; n != NULL; n = n->next)
	{
		SDL_Surface *ss = n->val;
		if(ss->w > combined.x) combined.x = ss->w;
		combined.y += ss->h;
	}
	ptr->size = combined;

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
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, combined.x, combined.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	GLuint clear_col[4] = { 0, 0, 0, 0 };
	glClearTexImage(ptr->fon_tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, &clear_col);

	float y = 0;
	for(n = ptr->surfs->start; n != NULL; n = n->next)
	{
		SDL_Surface *ss = n->val;
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, ss->w, ss->h, GL_BGRA, GL_UNSIGNED_BYTE, ss->pixels);
		y+=ss->h;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
}

void text2_set_position(text2 *ptr, float x, float y)
{
	ptr->pos.x = x;
	ptr->pos.y = y;
}

void text2_set_string(text2 *ptr, char *new_str)
{
	if(strs_are_equal(ptr->str, new_str)) return;
	if(ptr->str) free(ptr->str);
	ptr->str = dupe_str(new_str);
	text2_gen_surfaces(ptr);
}

void text2_set_color(text2 *ptr, color4 color)
{
	ptr->col = color;
}

void text2_set_opacity(text2 *ptr, float val)
{
	float rval = scale_value_to(clamp(val, 0.0f, 1.0f), 0.0f, 1.0f, 0, 255);
	ptr->col.a = rval;
	ptr->bg_col.a = rval;
}

void text2_add_surf(text2 *ptr, char *text, color4 col)
{
	SDL_Color rcol;
	rcol.r = col.r;
	rcol.g = col.g;
	rcol.b = col.b;
	rcol.a = col.a;
	SDL_Surface *add = TTF_RenderText_Blended(rg.font, text, rcol);

	if(add) list_push_back(ptr->surfs, add);
}

void text2_clear_surfs(text2 *ptr, char free_list)
{
	if(free_list) { list_free(ptr->surfs, SDL_FreeSurface); }
	else list_clear(ptr->surfs, SDL_FreeSurface);
}

void text2_free(text2 *ptr)
{
	glDeleteTextures(1, &(ptr->fon_tex));
	text2_clear_surfs(ptr, 1);
	free(ptr->str);
	free(ptr);
}
