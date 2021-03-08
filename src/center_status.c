#include "center_status.h"
#include "rg.h"

text *center_status_text;
timer *center_status_timer;

char center_status_lock = 0;
char center_status_first_msg = 1;

void show_center_status(char *message)
{
	if(center_status_first_msg) center_status_first_msg = 0;
	while(center_status_lock == 1) {}
	center_status_lock = 1;
	float ms = timer_milliseconds(center_status_timer);
	timer_restart(center_status_timer);
	text_set_string(center_status_text, message);
	center_status_lock = 0;
}

void center_status_free(char *message)
{
	show_center_status(message);
	free(message);
}

void render_center_status()
{
	if(!center_status_first_msg)
	{
		int center_status_time = timer_milliseconds(center_status_timer);
		if(center_status_time <= CENTER_STATUS_SHOW_TIME_MS)
		{
			text_set_position(center_status_text, rg.win_width_mid - (center_status_text->size.x / 2), (rg.win_height_mid + (rg.win_height / 4)) - (center_status_text->size.y / 2));
			text_render(center_status_text);
		}
	}
}