#pragma once
#include <cstdint>
#include "effect.hpp"
#include "esp_err.h"
#include "led_strip.h"
class PanelDriver {
public:
    esp_err_t initialize(int gpio, std::uint16_t panel_count = 1);
    esp_err_t show(PanelIndex panel, const FrameBuffer &frame);
private:
    led_strip_handle_t strip_{};
    std::uint16_t panel_count_{};
};
