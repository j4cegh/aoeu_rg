#include "mouse.h"

mouse_data mouse;

float gui_mouse_x;
float gui_mouse_y;
int left_click;
char left_click_released;
int middle_click;
char middle_click_released;
int right_click;
char right_click_released;
timer *left_click_held;
timer *middle_click_held;
timer *right_click_held;
SDL_SystemCursor cursor_type;
