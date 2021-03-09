#pragma once

#include "utils.h"

typedef struct
{
    int left;
    int top;
    int width;
    int height;
} int_rect;

typedef struct
{
    float left;
    float top;
    float width;
    float height;
} float_rect;

float_rect FLOATRECT(float left, float top, float width, float height);

#define FLOATRECTZERO FLOATRECT(0.0f, 0.0f, 0.0f, 0.0f)

#define FR_XYLOWEST(rect) FLOATRECT(return_lowest(rect.left, rect.width), return_lowest(rect.top, rect.height), return_highest(rect.left, rect.width), return_highest(rect.top, rect.height))

#define FR_CONTAINS(rect, x, y) \
    (x >= rect.left && x <= rect.width && y >= rect.top && y <= rect.height)

#define FR_CONTAINS2(rect, x, y) \
	(x >= rect.left && x <= rect.left + rect.width && y >= rect.top && y <= rect.top + rect.height)

#define FR_INTERSECTS_4P(rect, rect2) \
( \
    FR_CONTAINS(rect, rect2.left, rect2.top) || \
    FR_CONTAINS(rect, rect2.width, rect2.top) || \
    FR_CONTAINS(rect, rect2.width, rect2.height) || \
    FR_CONTAINS(rect, rect2.left, rect2.height) || \
    FR_CONTAINS(rect, rect2.left + ((rect2.width - rect2.left) / 2.0f), rect2.top + ((rect2.height - rect2.top) / 2.0f)) \
)
#define FR_INTERSECTS(rect, rect2) (FR_INTERSECTS_4P(rect, rect2) || FR_INTERSECTS_4P(FR_XYLOWEST(rect2), rect))
