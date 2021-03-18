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
