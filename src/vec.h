#pragma once

typedef struct v2f
{
    float x;
    float y;
} v2f;

v2f V2F(float x, float y);
#define V2FZERO V2F(0, 0)

float v2f_len_sq(v2f v);
float v2f_dist(v2f v1, v2f v2);
v2f *v2f_init(float x, float y);

typedef struct v2i
{
    int x;
    int y;
} v2i;

v2i V2I(int x, int y);
#define V2IZERO V2I(0, 0)

typedef struct v2u
{
    unsigned int x;
    unsigned int y;
} v2u;

v2u V2U(unsigned int x, unsigned int y);
