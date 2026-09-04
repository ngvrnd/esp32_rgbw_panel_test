#pragma once
#include <cstdint>
#include "frame_sink.hpp"
#include "esp_err.h"
#include "led_strip.h"
class PanelDriver : public FrameSink {
public:
    esp_err_t initialize(int gpio, std::uint16_t panel_count = 1);
    bool show(PanelIndex panel, const FrameBuffer &frame) override;
private:
    led_strip_handle_t strip_{};
    std::uint16_t panel_count_{};
};
