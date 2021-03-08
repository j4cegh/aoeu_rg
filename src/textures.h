#pragma once

#include "list.h"
#include "includes.h"

typedef enum
{
	tex_load_loc_res_root,
	tex_load_loc_filesystem
} tex_load_loc;

typedef struct
{
	char *name;
	SDL_Surface *surf;
	GLuint texture_id;
	v2f o;
	v2f sc;
	float sc_div;
	char no_smoothing;
	char flip_horiz;
} loaded_texture;

loaded_texture *texture_create(char *name, SDL_Surface *surf);
loaded_texture *texture_subsurf(loaded_texture *from, float_rect rect);
void texture_free_tex_and_surf(loaded_texture *ptr);
void texture_free(loaded_texture *ptr);

void textures_init();
SDL_Surface *textures_get_surf_from_file(char *f);
loaded_texture *textures_get(char *name, tex_load_loc load_loc, char reload);
void textures_draw(tex_load_loc load_loc, char *name, float x, float y, float width, float height, color4 col);
void textures_drawt(loaded_texture *tex, float x, float y, float width, float height, color4 col);
void textures_drawsc(loaded_texture *tex, float x, float y, float scalex, float scaley, color4 col);
void textures_free();

extern list *loaded_textures_list;
extern loaded_texture *missing_texture_ptr;
