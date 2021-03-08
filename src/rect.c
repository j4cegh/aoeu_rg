#include "rect.h"

float_rect FLOATRECT(float left, float top, float width, float height)
{
    float_rect ret;
    ret.left = left;
    ret.top = top;
    ret.width = width;
    ret.height = height;
    return ret;
}