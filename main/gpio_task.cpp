#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"

#include <uni.h>

#include "shared.h"
#include "board_config.h"
#include "gpio_task.h"

#define LED_BLINK_INTERVAL_MS 500
#define BT_RESET_DEBOUNCE_MS 1000

struct LEDInfo
{
    bool blink_state {false};
    unsigned long last_blink_time {0};
    gpio_num_t pin[MAX_GAMEPADS] {};
};

LEDInfo led_info;

void forget_keys_and_reset()
{
    uni_bt_del_keys_unsafe();
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

void check_reset_button()
{
    static unsigned long button_press_start_time = 0;
    static bool button_previously_pressed = false;

    unsigned long current_ms = esp_timer_get_time() / 1000;
    bool button_pressed = !gpio_get_level((gpio_num_t)RESET_BT_PIN);

    if (button_pressed && !button_previously_pressed) 
    {
        button_press_start_time = current_ms;
        button_previously_pressed = true;
    }
    else if (!button_pressed && button_previously_pressed) 
    {
        button_previously_pressed = false;
        button_press_start_time = 0;
    }

    unsigned long press_duration_ms = current_ms - button_press_start_time;

    if (button_pressed && press_duration_ms >= BT_RESET_DEBOUNCE_MS) 
    {
        forget_keys_and_reset();
        button_previously_pressed = false;
        button_press_start_time = 0;
    }
}

void init_reset_button()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << RESET_BT_PIN);
    io_conf.pull_up_en = (gpio_pullup_t)1;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    gpio_config(&io_conf);
}

void init_leds()
{
    const int led_indicator_pins[] = LED_INDICATOR_PINS;

    for (int i = 0; i < MAX_GAMEPADS; ++i) 
    {
        led_info.pin[i] = (gpio_num_t)led_indicator_pins[i];
        
        gpio_reset_pin(led_info.pin[i]);
        gpio_set_direction(led_info.pin[i], GPIO_MODE_OUTPUT);
        gpio_set_level(led_info.pin[i], 0);
    }
}

void gpio_task(void* param)
{
    (void)param;

    init_leds();
    init_reset_button();

    BP32Gamepad* gamepads[MAX_GAMEPADS];

    for (int i = 0; i < MAX_GAMEPADS; i++) 
    {
        gamepads[i] = get_gamepad(i);
    }

    while(1)
    {
        unsigned long current_ms = esp_timer_get_time() / 1000;

        if (current_ms - led_info.last_blink_time >= LED_BLINK_INTERVAL_MS) 
        {
            led_info.blink_state = !led_info.blink_state;
            led_info.last_blink_time = current_ms;

            for (int i = 0; i < MAX_GAMEPADS; i++) 
            {
                if (gamepads[i]->device_ptr == nullptr)
                {
                    gpio_set_level(led_info.pin[i], led_info.blink_state ? 1 : 0);
                }
                else
                {
                    gpio_set_level(led_info.pin[i], 1);

                }
            }
        }

        check_reset_button();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// void init_gpio()
// {
//     init_leds();
//     init_reset_button();
// }

// void gpio_task()
// {
//     unsigned long current_ms = esp_timer_get_time() / 1000;

//     if (current_ms - led_info.last_blink_time >= LED_BLINK_INTERVAL_MS) 
//     {
//         led_info.blink_state = !led_info.blink_state;
//         led_info.last_blink_time = current_ms;

//         for (int i = 0; i < MAX_GAMEPADS; i++) 
//         {
//             if (bp32_gamepad[i].device_ptr == nullptr)
//             {
//                 gpio_set_level(led_info.pin[i], led_info.blink_state ? 1 : 0);
//             }
//             else
//             {
//                 gpio_set_level(led_info.pin[i], 1);

//             }
//         }
//     }

//     check_reset_button();

//     vTaskDelay(pdMS_TO_TICKS(10));
// }