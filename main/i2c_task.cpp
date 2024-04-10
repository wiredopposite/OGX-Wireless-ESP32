#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "board_config.h"
#include "shared.h"
#include "i2c_task.h"

#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   400000

#define POLLING_RATE_MS 5

void init_i2c()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    printf("I2C Initialized.\n");
}

esp_err_t i2c_write(uint8_t device_address, const uint8_t* data, size_t data_len) 
{
    return i2c_master_write_to_device(I2C_MASTER_NUM, device_address, data, data_len, pdMS_TO_TICKS(50));
}

esp_err_t i2c_read(uint8_t device_address, uint8_t* data, size_t data_len) 
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
    
    if (data_len > 1) 
    {
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    }

    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

void i2c_task(void* param)
{
    (void) param;

    init_i2c();

    esp_err_t err;

    BP32Gamepad* gamepads[MAX_GAMEPADS];

    for (int i = 0; i < MAX_GAMEPADS; i++) 
    {
        gamepads[i] = get_gamepad(i);
    }

    while (1)
    {
        unsigned long start_ms = esp_timer_get_time() / 1000;

        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            if (gamepads[i]->device_ptr != nullptr)
            {
                if (gamepads[i]->new_out_packet)
                {
                    i2cOutPacket i2c_out_packet = gamepads[i]->i2c_out_packet;
                    
                    gamepads[i]->new_out_packet = false;

                    err = i2c_write(gamepads[i]->slave_address, (uint8_t*)&i2c_out_packet, sizeof(i2cOutPacket));

                    if (err != ESP_OK) 
                    {
                        #if (OGXW_DEBUG > 0)
                        printf("Error on I2C slave %d write: %s\n", gamepads[i]->slave_address, esp_err_to_name(err));
                        #endif

                        continue;
                    }
                }

                vTaskDelay(1);

                i2cInPacket i2c_in_packet;

                err = i2c_read(gamepads[i]->slave_address, (uint8_t*)&i2c_in_packet, sizeof(i2cInPacket));

                if (err == ESP_OK)
                {
                    process_in_packet(i, i2c_in_packet);
                }
                #if (OGXW_DEBUG > 0)
                else 
                {
                    printf("Error on I2C slave %d read: %s\n", gamepads[i]->slave_address, esp_err_to_name(err));
                }
                #endif

                vTaskDelay(1);
            }

            vTaskDelay(1);
        }

        unsigned long current_ms = esp_timer_get_time() / 1000;
        unsigned long length_ms = current_ms - start_ms;

        if (length_ms < POLLING_RATE_MS)
        {
            vTaskDelay(pdMS_TO_TICKS(POLLING_RATE_MS - length_ms));
        }
    }
}

// typedef struct {
//     int idx;
//     i2cInPacket in_packet;
// } ProcessInPacketParams;

// void processInPacketTask(void *pvParameters) {
//     ProcessInPacketParams *params = (ProcessInPacketParams *)pvParameters;
//     process_in_packet(params->idx, params->in_packet);

//     // Clean up allocated memory, if dynamically allocated in Step 3
//     free(pvParameters);

//     // Delete the task when finished
//     vTaskDelete(NULL);
// }