#include "ring_effect.h"
#ifndef RING_EFFECT_HOST_TEST
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#define WIDTH 8
#define HEIGHT 8
#define ACTIVE_WHITE 16
#define INNER_WHITE 1
#ifndef RING_EFFECT_HOST_TEST
static const char *TAG = "expanding_rings";
#endif

uint8_t ring_effect_index_for_xy(int x, int y)
{
    int dx = x < 3 ? 3 - x : (x > 4 ? x - 4 : 0);
    int dy = y < 3 ? 3 - y : (y > 4 ? y - 4 : 0);
    return (uint8_t)((dx > dy ? dx : dy) + 1);
}

ring_color_t ring_effect_default_color(uint8_t index, bool active)
{
    if (index == 0 || index > 4) return (ring_color_t){0, 0, 0, 0};
    return (ring_color_t){0, 0, 0, active ? ACTIVE_WHITE : INNER_WHITE};
}

static ring_color_t scaled(ring_color_t c, uint8_t mix)
{
    return (ring_color_t){(c.red*mix+127U)/255U,(c.green*mix+127U)/255U,
        (c.blue*mix+127U)/255U,(c.white*mix+127U)/255U};
}

ring_color_t ring_effect_color_for_pixel(int x, int y, uint8_t extent,
    uint8_t old_index, uint8_t old_mix, uint8_t new_index, uint8_t new_mix,
    ring_color_fn_t fn)
{
    if (!fn) fn = ring_effect_default_color;
    uint8_t index = ring_effect_index_for_xy(x, y);
    if (index == old_index) return scaled(fn(index, true), old_mix);
    if (index == new_index) return scaled(fn(index, true), new_mix);
    if (index < extent) return fn(index, false);
    return fn(0, false);
}

#ifndef RING_EFFECT_HOST_TEST

static void pace(void)
{
    static uint8_t phase; phase=(phase+1)%3;
    if (phase==0) vTaskDelay(1); else taskYIELD();
}

static void render(led_strip_handle_t panel,uint8_t extent,uint8_t oi,uint8_t om,
                   uint8_t ni,uint8_t nm,ring_color_fn_t fn)
{
    for(int y=0;y<HEIGHT;y++)for(int x=0;x<WIDTH;x++){
        ring_color_t c=ring_effect_color_for_pixel(x,y,extent,oi,om,ni,nm,fn);
        ESP_ERROR_CHECK(led_strip_set_pixel_rgbw(panel,y*WIDTH+x,c.red,c.green,c.blue,c.white));
    }
    ESP_ERROR_CHECK(led_strip_refresh(panel));
}

static void transition(led_strip_handle_t panel,uint8_t from,uint8_t to,
                       uint16_t dwell,ring_color_fn_t fn)
{
    uint8_t extent=from>to?from:to; ESP_LOGI(TAG,"Ring %u -> %u",from,to);
    for(uint16_t frame=0;frame<=dwell;frame++){
        uint8_t mix=(uint8_t)(frame*255U/dwell);
        render(panel,extent,from,255U-mix,to,mix,fn); pace();
    }
}

static void hold(led_strip_handle_t panel,uint8_t index,uint16_t dwell,ring_color_fn_t fn)
{
    ESP_LOGI(TAG,"Holding ring %u for %u frames",index,dwell);
    for(uint16_t frame=0;frame<dwell;frame++){render(panel,index,0,0,index,255,fn);pace();}
}

void expanding_rings_run(led_strip_handle_t panel,uint16_t inner_dwell,
                         uint16_t step_dwell,ring_color_fn_t fn)
{
    if(!fn)fn=ring_effect_default_color;
    if(!inner_dwell)inner_dwell=RING_EFFECT_DEFAULT_INNER_DWELL;
    if(!step_dwell)step_dwell=RING_EFFECT_DEFAULT_STEP_DWELL;
    ESP_LOGI(TAG,"Starting callable ring effect: inner=%u, step=%u",inner_dwell,step_dwell);
    while(true){
        for(uint8_t n=0;n<=ACTIVE_WHITE;n++){render(panel,1,0,0,1,n*255U/ACTIVE_WHITE,fn);pace();}
        hold(panel,1,inner_dwell,fn);
        transition(panel,1,2,step_dwell,fn);transition(panel,2,3,step_dwell,fn);
        transition(panel,3,4,step_dwell,fn);hold(panel,4,inner_dwell,fn);
        transition(panel,4,3,step_dwell,fn);transition(panel,3,2,step_dwell,fn);
        transition(panel,2,1,step_dwell,fn);hold(panel,1,inner_dwell,fn);
        for(int n=ACTIVE_WHITE;n>=0;n--){render(panel,1,0,0,1,n*255U/ACTIVE_WHITE,fn);pace();}
    }
}

void expanding_rings_run_default(led_strip_handle_t panel)
{
    expanding_rings_run(panel,RING_EFFECT_DEFAULT_INNER_DWELL,
        RING_EFFECT_DEFAULT_STEP_DWELL,ring_effect_default_color);
}
#endif
