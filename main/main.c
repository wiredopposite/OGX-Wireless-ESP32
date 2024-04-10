#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bp32_task.h"
#include "i2c_task.h"
#include "gpio_task.h"
#include "user_settings/user_settings_c.h"
#include "board_config.h"

#define STACK_MULTIPLIER (2048 * MAX_GAMEPADS)

#define BP32_STACK_SIZE (STACK_MULTIPLIER*16)
#define I2C_STACK_SIZE  (STACK_MULTIPLIER*8)
#define GPIO_STACK_SIZE (STACK_MULTIPLIER*2)

void app_main(void)
{
    init_user_settings();

    xTaskCreatePinnedToCore(
        gpio_task,
        "gpio",
        GPIO_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-5,
        NULL,
        1 );
        // tskNO_AFFINITY );

    xTaskCreatePinnedToCore(
        i2c_task,
        "i2c",
        I2C_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-3,
        NULL,
        1 );

    xTaskCreatePinnedToCore(
        bp32_task,
        "bp32",
        BP32_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-2,
        NULL,
        0 );
}