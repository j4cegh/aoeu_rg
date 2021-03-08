#pragma once

typedef struct color4
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} color4;

#define color4_2func(col) col.r, col.g, col.b
#define color4_2func_alpha(col) color4_2func(col), col.a

color4 color4_gen(unsigned char r, unsigned char g, unsigned char b);
color4 color4_gen_alpha(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
color4 color4_mod_alpha(color4 col, unsigned char a);

#define col_white       (color4_gen(255, 255, 255))
#define col_gray        (color4_gen(100, 100, 100))
#define col_black       (color4_gen(0, 0, 0))
#define col_yellow      (color4_gen(255, 255, 0))
#define col_green       (color4_gen(0, 255, 0))
#define col_red         (color4_gen(255, 0, 0))
#define col_brown       (color4_gen(165, 42, 42))
#define col_cyan        (color4_gen(0, 255, 255))
#define col_pink        (color4_gen(255, 192, 203))
#define col_transparent (color4_gen_alpha(0, 0, 0, 0))
