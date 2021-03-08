#pragma once

#include "includes.h"
#include "list.h"
#include "input_box.h"
#include "button.h"
#include "scroll_bar.h"
#include "text.h"

typedef enum widget_type
{
    widget_type_input_box,
    widget_type_button,
    widget_type_scroll_bar,
    widget_type_text
} widget_type;

typedef struct widget
{
    char *name;
    widget_type type;
    input_box *ib;
    button *b;
    scroll_bar *sb;
    text *t;
    float wanted_width;
} widget;

widget *widget_init(char *name, widget_type type);
void widget_free(widget *ptr);
void widget_tick(widget *ptr);

typedef enum ez_placement_state
{
    ez_placement_state_whole,
    ez_placement_state_split_row
} ez_placement_state;

extern list *widgets_list;
extern v2f ezgui_container_pos;
extern v2f ezgui_container_size;
extern v2f ezgui_container_padding;
extern ez_placement_state ezgui_placement_state;

void widgets_init();
void widgets_free();

void ez_cs(v2f pos, v2f size, v2f padding, char do_not_scissor);
void ez_ce();

void ez_w(); 
void ez_s();

float_rect ez_gbs(widget *w);

widget *ez_g(char *name);

widget *ez_pos(char *name, widget_type type);
input_box *ez_ib(char *name, char *initial_text);
button *ez_b(char *name, char *text);
button *ez_bp(char *name, char *str, float x, float y);
text *ez_t(char *name, char *str);
text *ez_tp(char *name, char *str, float x, float y, color4 col, char center_x, char center_y);
text *ez_tg(char *name, char *str, color4 col);
button *ez_bg(char *name, char *text);