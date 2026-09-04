#include "effect.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "expanding_rings.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "panel_driver.hpp"
namespace { constexpr int panel_gpio=16; constexpr PanelIndex panel_index=0; const char *tag="panel_demo";
void pace_frame(){static std::uint8_t phase{};phase=(phase+1)%3;if(phase==0)vTaskDelay(1);else taskYIELD();} }
extern "C" void app_main() {
    PanelDriver panel; ESP_ERROR_CHECK(panel.initialize(panel_gpio));
    FrameBuffer frame; ExpandingRings rings; Effect *effect=&rings;
    auto previous=static_cast<std::uint32_t>(esp_timer_get_time()/1000); effect->start(panel_index,previous);
    ESP_LOGI(tag,"Starting C++ effect '%s' on panel %u",effect->name(),panel_index);
    while(true){const auto now=static_cast<std::uint32_t>(esp_timer_get_time()/1000);
        effect->render(panel_index,{now,now-previous},frame);ESP_ERROR_CHECK(panel.show(panel_index,frame));
        previous=now;pace_frame();}
}
