
#include "mm_generic_backlight.h"
#include "mm_port_board_config.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "mm_err.h"

ledc_channel_config_t mm_ledc_channel;

#if !(MM_PORT_BL_GPIO >= 0)
#error MM_PORT_BL_GPIO must be non-negative
#endif

mm_err_t mm_generic_backlight_init(void)
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << MM_PORT_BL_GPIO
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = MM_PORT_BL_LS_MODE,
        .timer_num = MM_PORT_BL_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    mm_ledc_channel.channel    = MM_PORT_BL_HS_CHANNEL;
    mm_ledc_channel.duty       = 0;
    mm_ledc_channel.gpio_num   = MM_PORT_BL_GPIO;
    mm_ledc_channel.speed_mode = MM_PORT_BL_LS_MODE;
    mm_ledc_channel.timer_sel  = MM_PORT_BL_HS_TIMER;
    ledc_channel_config(&mm_ledc_channel);
    ledc_fade_func_install(0);

    return MM_OK;
}

mm_err_t mm_generic_backlight_set(uint8_t value)
{
    esp_err_t err;
    uint16_t duty;

    if(value == 0) {
        duty = 0;
    } else {
        //duty = LEDC_MAX_Duty-(32*(255-value));
        duty = MM_PORT_BL_MAX_Duty*value/255;
    }

    err = ledc_set_duty(mm_ledc_channel.speed_mode, mm_ledc_channel.channel, duty);
    if(err != ESP_OK) {
        MM_LOG_E("ledc_set_duty failed");
        return MM_FAIL;
    }
    err = ledc_update_duty(mm_ledc_channel.speed_mode, mm_ledc_channel.channel);
    if(err != ESP_OK) {
        MM_LOG_E("ledc_update_duty failed");
        return MM_FAIL;
    }

    return MM_OK;
}
