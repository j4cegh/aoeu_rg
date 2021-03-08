#include "gui_win_man.h"
#include "rg.h"
#include "utils.h"
#include "console.h"

list *gui_windows_list;
char gui_win_is_grabbed;

void gui_win_man_init()
{
	gui_windows_list = list_init();
	gui_win_is_grabbed = 0;
}

void gui_win_man_register(gui_win *ptr)
{
	list_push_back(gui_windows_list, ptr);
}

gui_win *gui_win_man_get_active()
{
	if(gui_windows_list->end) return (gui_win*)gui_windows_list->end->val;
	else return NULL;
}

void gui_win_man_render_windows()
{
	if(gui_windows_list->count < 1) return;

	gui_win_is_grabbed = 0;
	gui_win_hovered = 0;
	
	gui_win *cur = gui_win_man_get_active();

	list_node *n;
	for(n = gui_windows_list->start; n != NULL; n = n->next)
	{
		gui_win *wina = n->val;
		if(wina->visible) gui_win_render(wina);
	}
}

void gui_win_man_set_active(gui_win *ptr)
{
	gui_win *active = gui_win_man_get_active();
	if(active && (ptr == active || active->is_hovered)) return;
	list_node *n;
	for(n = gui_windows_list->end; n != NULL; n = n->before)
	{
		gui_win *lwin = n->val;
		if(lwin == ptr)
		{
			list_move_to_back(gui_windows_list, n);
			return;
		}
	}
}

void gui_win_man_free()
{
	list_free(gui_windows_list, gui_win_free);
}
