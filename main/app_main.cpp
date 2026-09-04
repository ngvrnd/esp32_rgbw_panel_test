#include "effect.hpp"
#include "effect_manager.hpp"
#include "digital_rain.hpp"
#include "debounced_button.hpp"
#include "driver/gpio.h"
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
    gpio_config_t button_config{};
    button_config.pin_bit_mask=1ULL<<GPIO_NUM_0;
    button_config.mode=GPIO_MODE_INPUT;
    button_config.pull_up_en=GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&button_config));

    ExpandingRings rings;
    DigitalRain rain;
    Effect *effects[]={&rain,&rings};
    std::size_t selected{};
    EffectManager manager(panel,1);
    const auto started=static_cast<std::uint64_t>(esp_timer_get_time());
    ESP_ERROR_CHECK(manager.assign(panel_index,effects[selected],started)?ESP_OK:ESP_FAIL);
    ESP_LOGI(tag,"Starting effect '%s' on panel %u; press BOOT to switch",effects[selected]->name(),panel_index);
    DebouncedButton button;
    while(true){const auto now=static_cast<std::uint64_t>(esp_timer_get_time());
        if(button.update(gpio_get_level(GPIO_NUM_0)==0,now)){
            selected=(selected+1)%2;
            ESP_ERROR_CHECK(manager.assign(panel_index,effects[selected],now)?ESP_OK:ESP_FAIL);
            ESP_LOGI(tag,"Selected effect '%s'",effects[selected]->name());
        }
        ESP_ERROR_CHECK(manager.render(now)?ESP_OK:ESP_FAIL);pace_frame();}
}
