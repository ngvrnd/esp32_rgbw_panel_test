#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef RING_EFFECT_HOST_TEST
typedef void *led_strip_handle_t;
#else
#include "led_strip.h"
#endif
typedef struct { uint8_t red, green, blue, white; } ring_color_t;
typedef ring_color_t (*ring_color_fn_t)(uint8_t index, bool is_ring_step);
enum { RING_EFFECT_DEFAULT_INNER_DWELL = 192, RING_EFFECT_DEFAULT_STEP_DWELL = 128 };
uint8_t ring_effect_index_for_xy(int x, int y);
ring_color_t ring_effect_default_color(uint8_t index, bool is_ring_step);
ring_color_t ring_effect_color_for_pixel(int x, int y, uint8_t inner_extent,
    uint8_t old_index, uint8_t old_mix, uint8_t new_index, uint8_t new_mix,
    ring_color_fn_t color_fn);
void expanding_rings_run(led_strip_handle_t panel, uint16_t inner_dwell,
                         uint16_t step_dwell, ring_color_fn_t color_fn);
void expanding_rings_run_default(led_strip_handle_t panel);
