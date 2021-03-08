#include "vec.h"
#include <math.h>
#include <stdlib.h>

v2f V2F(float x, float y)
{
    v2f ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

float v2f_len_sq(v2f v)
{
	return (v.x * v.x) + (v.y * v.y);
}

float v2f_dist(v2f v1, v2f v2)
{
	float num = v1.x - v2.x;
	float num2 = v1.y - v2.y;
	float num3 = num * num + num2 * num2;
	return (float) sqrt((double)num3);
}

v2f *v2f_init(float x, float y)
{
	v2f *ret = malloc(sizeof *ret);
	ret->x = x;
	ret->y = y;
	return ret;
}

v2i V2I(int x, int y)
{
    v2i ret;
    ret.x = x;
    ret.y = y;
    return ret;    
}

v2u V2U(unsigned int x, unsigned int y)
{
    v2u ret;
    ret.x = x;
    ret.y = y;
    return ret;   
}