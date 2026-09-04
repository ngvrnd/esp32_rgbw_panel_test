#include "panel_driver.hpp"
#include "led_strip_rmt.h"
esp_err_t PanelDriver::initialize(int gpio, std::uint16_t count) {
    led_strip_config_t strip{}; strip.strip_gpio_num=gpio; strip.max_leds=FrameBuffer::pixel_count*count;
    strip.led_model=LED_MODEL_SK6812; strip.color_component_format=LED_STRIP_COLOR_COMPONENT_FMT_GRBW;
    led_strip_rmt_config_t rmt{}; rmt.clk_src=RMT_CLK_SRC_DEFAULT; rmt.resolution_hz=10000000;
    rmt.mem_block_symbols=64; panel_count_=count;
    return led_strip_new_rmt_device(&strip,&rmt,&strip_);
}
esp_err_t PanelDriver::show(PanelIndex panel, const FrameBuffer &frame) {
    if (!strip_ || panel>=panel_count_) return ESP_ERR_INVALID_ARG;
    const auto offset=static_cast<std::size_t>(panel)*FrameBuffer::pixel_count;
    for(std::size_t i=0;i<frame.pixels().size();++i){const auto &c=frame.pixels()[i];
        const auto result=led_strip_set_pixel_rgbw(strip_,offset+i,c.red,c.green,c.blue,c.white);
        if(result!=ESP_OK)return result;}
    return led_strip_refresh(strip_);
}
