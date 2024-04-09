#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bp32_task.h"
#include "i2c_task.h"
#include "gpio_task.h"
#include "user_settings/user_settings_c.h"

#define BP32_STACK_SIZE 2048*18
#define I2C_STACK_SIZE 2048*8
#define GPIO_STACK_SIZE 2048*4

void app_main(void)
{
    init_user_settings();

    xTaskCreatePinnedToCore(
        bp32_task,
        "bp32",
        BP32_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-2,
        NULL,
        0 );

    xTaskCreatePinnedToCore(
        i2c_task,
        "i2c",
        I2C_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-3,
        NULL,
        1 );

    xTaskCreatePinnedToCore(
        gpio_task,
        "gpio",
        GPIO_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-5,
        NULL,
        1);
        // tskNO_AFFINITY );
}