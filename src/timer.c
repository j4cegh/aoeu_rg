#include "timer.h"
#ifdef __APPLE__
#include <mach/mach_time.h>
#else
#include <time.h>
#endif
#include <stdlib.h>
#include "list.h"
#include "utils.h"
#ifdef USING_WINDOWS
#include <windows.h>
#endif

TIMER_VAR se_get_microseconds()
{
#if defined(USING_WINDOWS)
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return 1000000 * time.QuadPart / frequency.QuadPart;
#elif defined(__APPLE__)
    static mach_timebase_info_data_t frequency = {0, 0};
    if(frequency.denom == 0) mach_timebase_info(&frequency);
    Uint64 nanoseconds = mach_absolute_time() * frequency.numer / frequency.denom;
    return nanoseconds / 1000;
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
