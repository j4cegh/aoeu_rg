#include "text3.h"
#include "glyphs.h"
#include "rg.h"
#include "color.h"

color4 se_text3_get_color_from_char(uint16_t c)
{
	color4 ret = col_white;
	if(c == 'w') ret = col_white;
	else if(c == 'h') ret = col_gray;
	else if(c == '0') ret = col_black;
	else if(c == 'u') ret = col_yellow;
	else if(c == 'a') ret = col_green;
	else if(c == 'r') ret = col_red;
	else if(c == 'p') ret = col_brown;
	else if(c == 'c') ret = col_cyan;
	else if(c == 'z') ret = col_pink;
	else if(c == 't') ret = col_transparent;
	return ret;
}

float line_widths[TEXT3_MAX_LINES];

float_rect text3_draw(char *str, float x, float y, float width_lim, float opacity, origin ox, origin oy, text3_justification just)
{
    glEnable(GL_TEXTURE_2D);

    size_t i;
    size_t sl = strlen(str);
	float msy = 0.0f;
	float sx = 0.0f, sy = 0.0f;
	float atx = 0.0f, aty = 0.0f;
	int line_count = 0;
	int line_index = 0;
	for(i = 0; i < sl; ++i)
    {
        uint16_t cc = str[i];
		if(cc == TEXT3_SPECIAL_CHAR)
		{
			++i;
			continue;
		}
        glyph *g = glyphs_get(cc);
        if(g != NULL)
        {
            v2f size = V2F(g->surf->w, g->surf->h);
			if(size.y > msy) msy = size.y;
            sx += size.x;
			if(sx > atx) atx = sx;
            if(sy + size.y > aty) aty = sy + size.y;
            if(sx >= (width_lim * 0.9) || cc == '\n' || i == sl - 1)
            {
				if(line_index < TEXT3_MAX_LINES) line_widths[line_index] = sx;
#ifndef RELEASE
				else show_notif("WARNING: text render exceeded maximum amount of lines!!!");
#endif
                sx = 0.0f;
                sy += msy;
				++line_count;
				++line_index;
            }
        }
    }
	color4 ccol = col_white;
	float orx = 0.0f, ory = 0.0f;
	switch(ox)
	{
		case origin_left: { orx = x; break; }
		case origin_right: { orx = x - atx; break; }
		case origin_center: { orx = x - (atx / 2); break; }
	}
	switch(oy)
	{
		case origin_top: { ory = y; break; }
		case origin_bottom: { ory = y - aty; break; }
		case origin_center: { ory = y - (aty / 2); break; }
	}
#define ORXP(orxp_var) (line_count > 1 ? (just == text3_justification_center ? ((atx / 2) - (line_widths[orxp_var] / 2)) : (just == text3_justification_right ? (atx - line_widths[orxp_var]) : 0.0f)) : 0.0f)
    float rx = orx + ORXP(0), ry = ory;
	float_rect trect = FLOATRECT(orx, ory, atx, aty);
#ifndef RELEASE
	if(rg.kp[SDL_SCANCODE_LCTRL]) GL_RECT2BOX(trect, color4_mod_alpha(col_black, 50));
#endif
	
	float maxhei = 0.0f;
	line_index = 0;
    for(i = 0; i < sl; ++i)
    {
        uint16_t cc = str[i];
		if(cc == TEXT3_SPECIAL_CHAR && i + 1 < sl)
		{
			uint16_t ccn = str[i + 1];
			ccol = se_text3_get_color_from_char(ccn);
			++i;
		}
        glyph *g = glyphs_get(cc);
        if(g != NULL)
        {
            v2f size = V2F(g->surf->w, g->surf->h);
            glyph_render_noglsc(g, rx, ry, color4_mod_alpha(ccol, opacity));
            rx += size.x;
            if(size.y > maxhei) maxhei = size.y;
            if(rx - orx >= (width_lim * 0.9f) || cc == '\n')
            {
				++line_index;
				float orxpv = ORXP(line_index);
                rx = orx + orxpv;
                ry += maxhei;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
#undef ORXP

    return trect;
}