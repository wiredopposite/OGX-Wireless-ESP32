#ifndef BP32_TASK_H_
#define BP32_TASK_H_

#include <uni.h>

#include "i2c_task.h"
#include "user_settings/user_settings.h"

struct BP32Gamepad
{
    uni_hid_device_t* device_ptr {nullptr};
    UserProfile profile;
    uint8_t input_mode_id {0};
    uint8_t slave_address;
    bool new_out_packet;
    i2cOutPacket i2c_out_packet;
    // i2cInPacket i2c_in_packet;
};

BP32Gamepad* get_gamepad(int idx);
void process_in_packet(int idx, i2cInPacket in_packet);

#endif // BP32_TASK_H_