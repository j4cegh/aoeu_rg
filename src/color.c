#include "color.h"

color4 color4_gen(unsigned char r, unsigned char g, unsigned char b)
{
    color4 ret;
    ret.r = r;
    ret.g = g;
    ret.b = b;
    ret.a = 255;
    return ret;
}

color4 color4_gen_alpha(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    color4 ret;
    ret.r = r;
    ret.g = g;
    ret.b = b;
    ret.a = a;
    return ret;
}

color4 color4_mod_alpha(color4 col, unsigned char a)
{
    color4 ret = col;
    ret.a = a;
    return ret;
}