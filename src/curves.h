#pragma once

#include "list.h"
#include "includes.h"

#define CURVES_CATMULL_DETAIL_LEVEL 30

void curves_limit_curve(list *curve_points, int limit);
list *curves_create_linear(list *control_points);
list *curves_create_catmull(list *control_points);
list *curves_create_perfect(list *control_points);
list *curves_create_bezier(list *control_points);
list *curves_create_bezier_3p(list *control_points);
void curves_delete_list(list *point_list);
