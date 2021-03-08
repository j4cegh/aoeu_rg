#pragma once

#include "list.h"
#include "rect.h"
#include "dir.h"

typedef enum gui_widget_type
{
	gui_widget_type_view,
	gui_widget_type_text,
	gui_widget_type_button,
	gui_widget_type_input_box,
	gui_widget_type_slider_bar,
	gui_widget_type_radio_buttons,
	gui_widget_type_scroll_bar,
	gui_widget_type_list
	/*gui_widget_type_*/
} gui_widget_type;

typedef struct gui_widget_view
{
	struct gui_widget *host_widget;

} gui_widget_view;

typedef struct gui_widget_text
{
	struct gui_widget *host_widget;

} gui_widget_text;

typedef struct gui_widget_button
{
	struct gui_widget *host_widget;

} gui_widget_button;

typedef struct gui_widget_input_box
{
	struct gui_widget *host_widget;

} gui_widget_input_box;

typedef struct gui_widget_slider_bar
{
	struct gui_widget *host_widget;

} gui_widget_slider_bar;

typedef struct gui_widget_radio_buttons
{
	struct gui_widget *host_widget;

} gui_widget_radio_buttons;

typedef struct gui_widget_scroll_bar
{
	struct gui_widget *host_widget;

} gui_widget_scroll_bar;

typedef struct gui_widget_list
{
	struct gui_widget *host_widget;

} gui_widget_list;

/*
typedef struct gui_widget_
{
	struct gui_widget *host_widget;

} gui_widget_;
*/

typedef struct gui_widget_mouse_state
{
	v2f pos; /* mouse pos relative to the gui widget pos */
	unsigned int left_mouse_state;
	unsigned int middle_mouse_state;
	unsigned int right_mouse_state;
	char hovered;
} gui_widget_mouse_state;

typedef struct gui_widget_theme
{
	color4 foreground_color;
	color4 background_color;
	int font_size;
} gui_widget_theme;

typedef struct gui_widget
{
	struct gui_widget *parent; /* can be NULL */
	char *tooltip_msg; /* can be NULL */
	v2f pos;
	v2f size;
	v2f size_limit;
	origin origin;
	unsigned char opacity; /* 0 - 255 */
	char enabled;
	gui_widget_type type;
	gui_widget_theme theme;
	/* all of these widgets can be NULL */
	struct gui_widget_view *view;
	struct gui_widget_text *text;
	struct gui_widget_button *button;
	struct gui_widget_input_box *input_box;
	struct gui_widget_slider_bar *slider_bar;
	struct gui_widget_radio_buttons *radio_buttons;
	struct gui_widget_scroll_bar *scroll_bar;
	struct gui_widget_list *list;
	list *children;
	gui_widget_mouse_state mouse_state;
} gui_widget;

gui_widget *gui_widget_init(gui_widget_type type);
void gui_widget_free(gui_widget *ptr);
void gui_widget_set_pos(gui_widget *ptr, v2f pos);
void gui_widget_set_size(gui_widget *ptr, v2f size);
void gui_widget_set_opacity(gui_widget *ptr, unsigned char opacity);
void gui_widget_set_tooltip(gui_widget *ptr, char *msg);
void gui_widget_add_child(gui_widget *ptr, gui_widget *child);

void gui_widget_update(gui_widget *ptr);
void gui_widget_render(gui_widget *ptr);