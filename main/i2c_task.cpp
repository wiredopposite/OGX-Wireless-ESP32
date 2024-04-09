#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "board_config.h"
#include "shared.h"
#include "i2c_task.h"

#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   400000

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
    return i2c_master_write_to_device(I2C_MASTER_NUM, device_address, data, data_len, pdMS_TO_TICKS(1000));
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

    static i2cOutPacket prev_out_packet[MAX_GAMEPADS] = {};
    esp_err_t err;

    while (1)
    {
        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            if (bp32_gamepad[i].device_ptr != nullptr)
            {
                if (memcmp(&bp32_gamepad[i].i2c_out_packet, &prev_out_packet[i], sizeof(i2cOutPacket)) != 0)
                {
                    err = i2c_write(bp32_gamepad[i].slave_address, (const uint8_t*)&bp32_gamepad[i].i2c_out_packet, sizeof(i2cOutPacket));

                    vTaskDelay(pdMS_TO_TICKS(1));

                    if (err != ESP_OK) 
                    {
                        // printf("Error on I2C slave %d write: %s\n", bp32_gamepad[i].slave_address, esp_err_to_name(err));
                        vTaskDelay(pdMS_TO_TICKS(1));
                        continue;
                    }

                    prev_out_packet[i] = bp32_gamepad[i].i2c_out_packet;
                }

                i2cInPacket i2c_in_packet;

                err = i2c_read(bp32_gamepad[i].slave_address, (uint8_t*)&i2c_in_packet, sizeof(i2cInPacket));

                if (err == ESP_OK)
                {
                    process_in_packet(i, i2c_in_packet);
                }
                
                // if (err != ESP_OK) 
                // {
                //     printf("Error on I2C slave %d read: %s\n", bp32_gamepad[i].slave_address, esp_err_to_name(err));
                // }
            }

            vTaskDelay(pdMS_TO_TICKS(1));
        }

        vTaskDelay(1);
    }
}