#include "esp_log.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "ring_effect.h"
#define PANEL_GPIO 16
void app_main(void)
{
    led_strip_config_t strip={.strip_gpio_num=PANEL_GPIO,.max_leds=64,
        .led_model=LED_MODEL_SK6812,.color_component_format=LED_STRIP_COLOR_COMPONENT_FMT_GRBW,
        .flags.invert_out=false};
    led_strip_rmt_config_t rmt={.clk_src=RMT_CLK_SRC_DEFAULT,.resolution_hz=10000000,
        .mem_block_symbols=64,.flags.with_dma=false};
    led_strip_handle_t panel=NULL;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip,&rmt,&panel));
    expanding_rings_run_default(panel);
}
