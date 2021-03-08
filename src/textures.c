#include "textures.h"
#include "utils.h"
#include "logger.h"
#include "rg.h"

list *loaded_textures_list;
loaded_texture *missing_texture_ptr;

loaded_texture *texture_create(char *name, SDL_Surface *surf)
{
	loaded_texture *ret = malloc(sizeof *ret);
	ret->name = dupe_str(name);
	ret->surf = surf;
	ret->texture_id = 0;
	ret->o = V2FZERO;
	ret->sc = V2F(1.0f, 1.0f);
	ret->sc_div = 1;
	ret->no_smoothing = 0;
	ret->flip_horiz = 0;
	return ret;
}

loaded_texture *texture_subsurf(loaded_texture *from, float_rect rect)
{
	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
	SDL_Surface *surf = SDL_CreateRGBSurface(0, rect.width, rect.height, 32, rmask, gmask, bmask, amask);
	SDL_Rect src_rect = { rect.left, rect.top, rect.width, rect.height }, dst_rect = { 0, 0, rect.width, rect.height };
	SDL_BlitSurface(from->surf, &src_rect, surf, &dst_rect);
	loaded_texture *ret = texture_create("?", surf);
	list_push_back(loaded_textures_list, ret);
	return ret;
}

void texture_free_tex_and_surf(loaded_texture *ptr)
{
	if(ptr->texture_id != 0) glDeleteTextures(1, &(ptr->texture_id));
	if(!(ptr->surf == missing_texture_ptr->surf && ptr != missing_texture_ptr)) SDL_FreeSurface(ptr->surf);	
	ptr->texture_id = 0;
}

void texture_free(loaded_texture *ptr)
{
	texture_free_tex_and_surf(ptr);
	free(ptr->name);
	free(ptr);
}

void textures_init()
{
	loaded_textures_list = list_init();

	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

	missing_texture_ptr = texture_create("se_missing_tex", SDL_CreateRGBSurface(0, 1, 1, 32, rmask, gmask, bmask, amask));
}

SDL_Surface *textures_get_surf_from_file(char *f)
{
	SDL_Surface *ret = IMG_Load(f);
	if(!ret)
	{
		char *missing_tex_warning = dupfmt("missing texture! filename: %s", f);
		show_notif(missing_tex_warning);
		log_warning(missing_tex_warning);
		printf(missing_tex_warning);
		free(missing_tex_warning);
		return missing_texture_ptr->surf;
	}
	return ret;
}

loaded_texture *textures_get(char *name, tex_load_loc load_loc, char reload)
{
	char *loc = NULL;
	if(load_loc == tex_load_loc_res_root) loc = dupfmt(RES_DIR "%s", name);
	else if(load_loc == tex_load_loc_filesystem) loc = name;
	loaded_texture *ret = NULL;

	if(loc)
	{
		list_node *n;

		for(n = loaded_textures_list->start; n != NULL; n = n->next)
		{
			loaded_texture *lt = n->val;
			if(strs_are_equal(loc, lt->name))
			{
				if(reload)
				{
					texture_free_tex_and_surf(lt);
					lt->surf = textures_get_surf_from_file(loc);
				}
				ret = lt;
				break;
			}
		}

		if(!ret)
		{
			ret = texture_create(loc, textures_get_surf_from_file(loc));
			list_push_back(loaded_textures_list, ret);
		}

		if(load_loc != tex_load_loc_filesystem) free(loc);
	}

	if(ret == NULL) ret = missing_texture_ptr;
	return ret;
}

void textures_draw(tex_load_loc load_loc, char *name, float x, float y, float width, float height, color4 col)
{
	loaded_texture *tex = textures_get(name, load_loc, 0);
	textures_drawt(tex, x, y, width, height, col);
}

void textures_drawt(loaded_texture *tex, float x, float y, float width, float height, color4 col)
{
	glEnable(GL_TEXTURE_2D);
	SDL_Surface *surface = tex->surf;
	
	if(tex->texture_id == 0)
	{
		glGenTextures(1, &(tex->texture_id));
		glBindTexture(GL_TEXTURE_2D, tex->texture_id);
		int texture_format = GL_RGBA;
		int colors_num = surface->format->BytesPerPixel;
		if(colors_num == 4)
		{
			if(surface->format->Rmask == 0x000000ff) texture_format = GL_RGBA;
			else texture_format = GL_BGRA;
		}
		else if(colors_num == 3)
		{
			if(surface->format->Rmask == 0x000000ff) texture_format = GL_RGB;
			else texture_format = GL_BGR;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, texture_format, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);

		if(tex->no_smoothing)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	else glBindTexture(GL_TEXTURE_2D, tex->texture_id);

	if(width == -1) width = surface->w;
	if(height == -1) height = surface->h;
	x -= tex->o.x * tex->sc.x;
	y -= tex->o.y * tex->sc.y;

	glColor4ub(color4_2func_alpha(col));
	
	if(tex->flip_horiz)
	{
		glBegin(GL_QUADS);
			glTexCoord2f(1, 0); glVertex3f(x, y, 0);
			glTexCoord2f(0, 0); glVertex3f(x + width, y, 0);
			glTexCoord2f(0, 1); glVertex3f(x + width, y + height, 0);
			glTexCoord2f(1, 1); glVertex3f(x, y + height, 0);
		glEnd();
	}
	else 
	{
		glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex3f(x, y, 0);
			glTexCoord2f(1, 0); glVertex3f(x + width, y, 0);
			glTexCoord2f(1, 1); glVertex3f(x + width, y + height, 0);
			glTexCoord2f(0, 1); glVertex3f(x, y + height, 0);
		glEnd();
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

void textures_drawsc(loaded_texture *tex, float x, float y, float scalex, float scaley, color4 col)
{
	tex->sc.x = scalex / tex->sc_div;
	tex->sc.y = scaley / tex->sc_div;
	textures_drawt(tex, x, y, tex->surf->w * tex->sc.x, tex->surf->h * tex->sc.y, col);
}

void textures_free()
{
	list_free(loaded_textures_list, texture_free);
	texture_free(missing_texture_ptr);
}
