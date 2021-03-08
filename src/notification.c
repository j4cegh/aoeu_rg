#include "notification.h"
#include "rg.h"

list *notifications_list;

void show_notif_col(char *msg, color4 col)
{
	notification *noti = (notification*) malloc(sizeof *noti);
	noti->msg = dupe_str(msg);
	noti->col = col;
	noti->timer = timer_init();
	noti->t = text_init(noti->msg, 0, 0, col_white);
	noti->height = (noti->t->size.y);
	list_push_back(notifications_list, noti);
}

void show_notif(char *msg)
{
#define RR rand_num_ranged(50, 255)
	show_notif_col(msg, color4_gen(RR, RR, RR));
#undef RR
}

void show_notif_free(char *msg)
{
	show_notif(msg);
	free(msg);
}

void notif_delete(notification *noti)
{
	text_free(noti->t);
	timer_free(noti->timer);
	free(noti->msg);
	free(noti);
}

void render_notifs()
{
	if(notifications_list->count > 0)
	{
		float yup_off = -2;
		float lift_progress = 0;
		list_node *nn;
		for(nn = notifications_list->end; nn != NULL; nn = nn->before)
		{
			notification *not = nn->val;
			char is_end = (nn == notifications_list->end);
			int time_ms = timer_milliseconds(not->timer);
			if(is_end) lift_progress = clamp(scale_value_to(time_ms, 0, NOTIF_RISE_MS, not->height, 0), 0, not->height);
			yup_off += not->height;

#define MAX_OP 60
			float op = clamp(scale_value_to(time_ms, NOTIF_SHOW_TIME_MS - NOTIF_RISE_MS, NOTIF_SHOW_TIME_MS, MAX_OP, 0), 0, MAX_OP);
			not->col.a = op;

			text_set_opacity(not->t, scale_value_to(op, 0, MAX_OP, 0, 1));
			v2f pos = V2F(rg.win_width - not->t->size.x - scale_value_to(op, MAX_OP, 0, 0, 50), (rg.win_height + lift_progress) - yup_off);
#undef MAX_OP
			text_set_position(not->t, pos.x, pos.y);
			
			glColor4ub(color4_2func_alpha(not->col));
			glBegin(GL_QUADS);
			glVertex2f(not->t->pos.x, not->t->pos.y);
			glVertex2f(not->t->pos.x + not->t->size.x, not->t->pos.y);
			glVertex2f(not->t->pos.x + not->t->size.x, not->t->pos.y + not->t->size.y);
			glVertex2f(not->t->pos.x, not->t->pos.y + not->t->size.y);
			glEnd();

			text_render(not->t);
		}
		/*
 		while(notifications_list->count > (rg.win_height / 20))
		{
			notif_delete(notifications_list->start->val);
			list_pop_front(notifications_list);
		}
		*/
	}
}