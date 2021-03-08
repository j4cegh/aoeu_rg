#include "curves.h"
#include "utils.h"
#include "slider.h"
#include "rg.h"

void curves_limit_curve(list *curve_points, int bef_limit)
{
	if(curve_points->count <= 0) return;
	int limit = clamp(bef_limit, 1, INFINITY);
	while(curve_points->count % limit != 0)
	{
		free(curve_points->end->val);
		list_pop_back(curve_points);
	}
}

list *curves_create_linear(list *control_points)
{
	if(control_points == NULL) return NULL;

	list *ret_list = list_init();

	if(control_points->count >= 2)
	{
		int i;
		for(i = 1; i < control_points->count; i++)
		{
			slider_control_point *lp = list_get_val_at(control_points, i - 1);
			slider_control_point *cp = list_get_val_at(control_points, i);

			float sc = get_distf(lp->pos.x, lp->pos.y, cp->pos.x, cp->pos.y);
			int cpi;
			for(cpi = 0; cpi < sc; cpi++)
			{
				float div = ((float) cpi / sc);
				list_push_back(ret_list, v2f_init(lp->pos.x + (cp->pos.x - lp->pos.x) * div, lp->pos.y + (cp->pos.y - lp->pos.y) * div));
			}
		}
	}

	return ret_list;
}

list *curves_create_catmull(list *control_points)
{
	if(control_points == NULL) return NULL;
	if(control_points->count <= 3) return curves_create_linear(control_points);

	list *ret = list_init();

	int j;
	for(j = 0; j < control_points->count - 1; j++)
	{
		v2f v1 = ((slider_control_point*) (j - 1 >= 0 ? list_get_val_at(control_points, j - 1) : list_get_val_at(control_points, j)))->pos;
		v2f v2 = ((slider_control_point*) list_get_val_at(control_points, j))->pos;
		v2f v3a = V2F(v2.x + (v2.x - v1.x), v2.y + (v2.y - v1.y));
		v2f v3 = ((slider_control_point*) (j + 1 < control_points->count ? list_get_val_at(control_points, j + 1) : &v3a))->pos;
		v2f v4a = V2F(v3.x + (v3.x - v2.x), v3.y + (v3.y - v2.y));
		v2f v4 = ((slider_control_point*) (j + 2 < control_points->count ? list_get_val_at(control_points, j + 2) : &v4a))->pos;

		int k;
		for(k = 0; k < CURVES_CATMULL_DETAIL_LEVEL; k++)
		{
			float amount = (float) k / CURVES_CATMULL_DETAIL_LEVEL;
			float num = amount * amount;
			float num2 = amount * num;

			float x = 0.5f * ((((2.0f * v2.x) + ((-v1.x + v3.x) * amount)) + (((((2.0f * v1.x) - (5.0f * v2.x)) + (4.0f * v3.x)) - v4.x) * num)) + ((((-v1.x + (3.0f * v2.x)) - (3.0f * v3.x)) + v4.x) * num2));
			float y = 0.5f * ((((2.0f * v2.y) + ((-v1.y + v3.y) * amount)) + (((((2.0f * v1.y) - (5.0f * v2.y)) + (4.0f * v3.y)) - v4.y) * num)) + ((((-v1.y + (3.0f * v2.y)) - (3.0f * v3.y)) + v4.y) * num2));

			list_push_back(ret, v2f_init(x, y));
		}
	}

	list *real_ret = curves_create_linear(ret);

	curves_delete_list(ret);

	return real_ret;
}

double circle_t_at(v2f pt, v2f center)
{
	return atan2(pt.y - center.y, pt.x - center.x);
}

list *curves_create_perfect(list *control_points)
{
	if(control_points->count < 3) return curves_create_linear(control_points);
	else if(control_points->count > 3) return curves_create_bezier(control_points);

	v2f p1 = ((slider_control_point*) list_get_val_at(control_points, 0))->pos;
	v2f p2 = ((slider_control_point*) list_get_val_at(control_points, 1))->pos;
	v2f p3 = ((slider_control_point*) list_get_val_at(control_points, 2))->pos;

	if(is_straight_line(p1, p2, p3)) return curves_create_bezier_3p(control_points);

	list *ret = list_init();
	
	v2f center;
	float radius;
	double t_initial, t_final;

	float circle_pi = 3.14159274f;
	float D = 2 * (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y));

	float p1_mag_sq = v2f_len_sq(p1);
	float p2_mag_sq = v2f_len_sq(p2);
	float p3_mag_sq = v2f_len_sq(p3);

	center.x = (p1_mag_sq * (p2.y - p3.y) + p2_mag_sq * (p3.y - p1.y) + p3_mag_sq * (p1.y - p2.y)) / D;
	center.y = (p1_mag_sq * (p3.x - p2.x) + p2_mag_sq * (p1.x - p3.x) + p3_mag_sq * (p2.x - p1.x)) / D;
	radius = v2f_dist(center, p1);

	t_initial = circle_t_at(p1, center);
	double t_mid = circle_t_at(p2, center);
	t_final = circle_t_at(p3, center);

	while(t_mid < t_initial) t_mid += 2 * circle_pi;
	while(t_final < t_initial) t_final += 2 * circle_pi;
	if(t_mid > t_final) t_final -= 2 * circle_pi;

	float curve_len = fabs((t_final - t_initial) * radius);
	int segments = (int)(curve_len * 0.125f);

	int i;
	for(i = 1; i < segments; i++)
	{
		double progress = (double) i / (double) segments;
		double t = t_final * progress + t_initial * (1 - progress);
		list_push_back(ret, v2f_init((float)(cos(t) * radius) + center.x, (float)(sin(t) * radius) + center.y));
	}

	list *real_ret = curves_create_linear(ret);

	curves_delete_list(ret);

	return real_ret;
}

float lerp2f(float first, float second, float by)
{
	return first * (1 - by) + second * by;
}

void lerp2v(v2f *first, v2f *second, float amount, v2f *out)
{
	*out = V2F(lerp2f(first->x, second->x, amount), lerp2f(first->y, second->y, amount));
}

list *curves_create_bezier(list *control_points)
{
	if(control_points == NULL) return NULL;
	if(control_points->count < 3) return curves_create_linear(control_points);
	if(control_points->count == 3) return curves_create_perfect(control_points);

	list *ret_list = list_init();

	list *working = list_init();
	
	list_node *n;
	for(n = control_points->start; n != NULL; n = n->next)
	{
		v2f *pt = n->val;
		list_push_back(working, v2f_init(pt->x, pt->y));
	}

	int points = 13 * control_points->count;

	for(int iteration = 0; iteration < points; iteration++)
	{
		for(int i = 0; i < control_points->count; i++)
		{
			v2f *w = list_get_val_at(working, i);
			v2f *wc = list_get_val_at(control_points, i);
			*w = *wc;
		}

		for(int level = 0; level < control_points->count; level++)
		{
			for (int i = 0; i < control_points->count - level - 1; i++)
			{
				v2f *w = list_get_val_at(working, i);
				v2f *wp1 = list_get_val_at(working, i + 1);
				lerp2v(w, wp1, (float)iteration / points, w);
			}
		}

		v2f *w = list_get_val_at(working, 0);
		list_push_back(ret_list, v2f_init(w->x, w->y));
	}

	curves_delete_list(working);

	list *rl2 = curves_create_linear(ret_list);

	curves_delete_list(ret_list);

	return rl2;
}

list *curves_create_bezier_3p(list *control_points)
{
	list *ret = NULL;
	if(control_points->count == 3)
	{
		list *ncps = list_init();
		list_node *n;
		
		for(n = control_points->start; n != NULL; n = n->next)
		{
			slider_control_point *scp = n->val;
			list_push_back(ncps, slider_control_point_init(scp->pos.x, scp->pos.y));
			if(n == control_points->end) list_push_back(ncps, slider_control_point_init(scp->pos.x, scp->pos.y));
		}
		
		ret = curves_create_bezier(ncps);

		list_free(ncps, slider_control_point_free);
	}
	return ret;
}

void curves_delete_list(list *point_list)
{
	list_free(point_list, free);
}
