#pragma once

#define TIMER_TIME_MODIFIER 1
#define TIMER_VAR unsigned long long int

typedef struct timer
{
    char *name;
    TIMER_VAR microseconds;
} timer;

timer *timer_init();
void timer_free(timer *t);
TIMER_VAR timer_microseconds(timer *t);
float timer_milliseconds(timer *t);
float timer_seconds(timer *t);
void timer_set_microseconds(timer *t, TIMER_VAR microseconds);
void timer_set_milliseconds(timer *t, float milliseconds);
void timer_set_seconds(timer *t, float seconds);
float timer_loop(timer *t, float milliseconds);
char timer_bool(timer *t, float milliseconds);
void timer_restart(timer *t);

extern struct list *timer_list;

void timer_mgr_init();
void timer_mgr_free();
timer *timer_find(char *name);
TIMER_VAR timer_name_microseconds(char *name);
float timer_name_milliseconds(char *name);
float timer_name_seconds(char *name);
void timer_name_set_microseconds(char *name, TIMER_VAR microseconds);
void timer_name_set_milliseconds(char *name, float milliseconds);
void timer_name_set_seconds(char *name, float seconds);
float timer_name_loop(char *name, float milliseconds);
char timer_name_bool(char *name, float milliseconds);
void timer_name_restart(char *name);
