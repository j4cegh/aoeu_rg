#include "timer.h"
#include <time.h>
#include <stdlib.h>
#include "list.h"
#include "utils.h"
#ifdef USING_WINDOWS
#include <windows.h>
#endif

TIMER_VAR se_get_microseconds()
{
#ifdef USING_WINDOWS
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return 1000000 * time.QuadPart / frequency.QuadPart;
#else
    struct timespec time;
    clock_gettime(1, &time);
    return (time.tv_sec * 1000000) + time.tv_nsec / 1000;
#endif
}

timer *timer_init()
{
    timer *ret = malloc(sizeof *ret);
    ret->name = NULL;
    ret->microseconds = se_get_microseconds();
    return ret;
}

void timer_free(timer *t)
{
    if(t->name != NULL) free(t->name);
    free(t);
}

TIMER_VAR timer_microseconds(timer *t)
{
    return (se_get_microseconds() - t->microseconds) * TIMER_TIME_MODIFIER;
}

float timer_milliseconds(timer *t)
{
    return timer_microseconds(t) / 1000.0f;
}

float timer_seconds(timer *t)
{
    return timer_milliseconds(t) / 1000.0f;
}

void timer_set_microseconds(timer *t, TIMER_VAR microseconds)
{
    t->microseconds = microseconds;
}

void timer_set_milliseconds(timer *t, float milliseconds)
{
    t->microseconds = ((TIMER_VAR) milliseconds) * 1000;
}

void timer_set_seconds(timer *t, float seconds)
{
    t->microseconds = (((TIMER_VAR) seconds) * 1000) * 1000;
}

float timer_loop(timer *t, float milliseconds)
{
    float ret = timer_milliseconds(t);
    if(ret >= milliseconds) timer_restart(t);
    return ret;
}

char timer_bool(timer *t, float milliseconds)
{
    if(timer_milliseconds(t) >= milliseconds)
    {
        timer_restart(t);
        return 1;
    }
    return 0;
}

void timer_restart(timer *t)
{
    t->microseconds = se_get_microseconds();
}

struct list *timer_list;

void timer_mgr_init()
{
    timer_list = list_init();
}

void timer_mgr_free()
{
    list_free(timer_list, timer_free);
}

timer *timer_find(char *name)
{
    list_node *n;
    timer *found = NULL;

    for(n = timer_list->start; n != NULL; n = n->next)
    {
        timer *t = (timer*) n->val;
        if(name != NULL && strs_are_equal(name, t->name))
        {
            found = t;
            break;
        }
    }

    if(found == NULL)
    {
        timer *new = timer_init();
        new->name = dupe_str(name);
        list_push_back(timer_list, new);
        found = new;
    }

    return found;
}

TIMER_VAR timer_name_microseconds(char *name)
{
    return timer_microseconds(timer_find(name));
}

float timer_name_milliseconds(char *name)
{
    return timer_milliseconds(timer_find(name));
}

float timer_name_seconds(char *name)
{
    return timer_seconds(timer_find(name));
}

void timer_name_set_microseconds(char *name, TIMER_VAR microseconds)
{
    timer_set_microseconds(timer_find(name), microseconds);
}

void timer_name_set_milliseconds(char *name, float milliseconds)
{
    timer_set_milliseconds(timer_find(name), milliseconds);
}

void timer_name_set_seconds(char *name, float seconds)
{
    timer_set_seconds(timer_find(name), seconds);
}

float timer_name_loop(char *name, float milliseconds)
{
    return timer_loop(timer_find(name), milliseconds);
}

char timer_name_bool(char *name, float milliseconds)
{
    return timer_bool(timer_find(name), milliseconds);
}

void timer_name_restart(char *name)
{
    timer_restart(timer_find(name));
}
