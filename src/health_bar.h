#pragma once

#define HEALTH_BAR_VAL_MIN 0.0f
#define HEALTH_BAR_VAL_MAX 100.0f

typedef struct health_bar
{
    float val;
    float rendval;
    struct song *song;
} health_bar;

void health_bar_reset(health_bar *ptr, struct song *song);
void health_bar_add(health_bar *ptr, float amount);
void health_bar_sub(health_bar *ptr, float amount);
void health_bar_clamp(health_bar *ptr);
void health_bar_update(health_bar *ptr);
void health_bar_render(health_bar *ptr);
