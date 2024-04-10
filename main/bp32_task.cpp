#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>
#include <uni.h>

#include "ble_server/ble_server.h"
#include "user_settings/user_settings.h"
#include "user_settings/user_settings_c.h"
#include "utilities/scaling.h"
#include "board_config.h"
#include "bp32_task.h"
#include "shared.h"
#include "i2c_task.h"

BP32Gamepad bp32_gamepad[MAX_GAMEPADS];

BP32Gamepad* get_gamepad(int idx)
{
    return &bp32_gamepad[idx];
}

//
// Platform Overrides/Callbacks
//
void my_platform_init(int argc, const char** argv) 
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

#if 0
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
#endif
    //    uni_bt_service_set_enabled(true);
}

void my_platform_on_init_complete(void) 
{
    logi("Bluepad32 init complete.\n");

    uni_bt_enable_new_connections_unsafe(true);

    if (1) uni_bt_del_keys_unsafe();
}

void my_platform_on_device_connected(uni_hid_device_t* d) 
{
    logi("Device connected: %p\n", d);
}

void my_platform_on_device_disconnected(uni_hid_device_t* d) 
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (bp32_gamepad[i].device_ptr == d)
        {
            bp32_gamepad[i].device_ptr = nullptr;
            bp32_gamepad[i].slave_address = 10;
            bp32_gamepad[i].i2c_out_packet = {0};
            break;
        }
    }

    logi("Device disconnected: %p\n", d);
}

uni_error_t my_platform_on_device_ready(uni_hid_device_t* d)
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (bp32_gamepad[i].device_ptr == NULL)
        {
            bp32_gamepad[i].device_ptr = d;
            bp32_gamepad[i].slave_address = i + 1;
            break;
        }
    }

    return UNI_ERROR_SUCCESS;
}

void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) 
{
    static uni_gamepad_t* prev_gamepad[MAX_GAMEPADS] = {};
    i2cOutPacket local_out_packet = {};
    int idx = 0;

    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (bp32_gamepad[i].device_ptr == d)
        {
            idx = i;
            break;
        }
    }

    if (memcmp(&prev_gamepad[idx], &ctl->gamepad, sizeof(uni_gamepad_t)) == 0) 
    {
        return;
    }

    uni_gamepad_t* bt_gamepad;

    switch (ctl->klass) 
    {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            bt_gamepad = &ctl->gamepad;
            prev_gamepad[idx] = bt_gamepad;

            if (bt_gamepad->dpad & DPAD_DOWN ) local_out_packet.buttons |= bp32_gamepad[idx].profile.down; 
            if (bt_gamepad->dpad & DPAD_LEFT ) local_out_packet.buttons |= bp32_gamepad[idx].profile.left; 
            if (bt_gamepad->dpad & DPAD_RIGHT) local_out_packet.buttons |= bp32_gamepad[idx].profile.right; 
            if (bt_gamepad->dpad & DPAD_UP   ) local_out_packet.buttons |= bp32_gamepad[idx].profile.up; 

            if (bt_gamepad->buttons & BUTTON_A) local_out_packet.buttons |= bp32_gamepad[idx].profile.a; 
            if (bt_gamepad->buttons & BUTTON_B) local_out_packet.buttons |= bp32_gamepad[idx].profile.b; 
            if (bt_gamepad->buttons & BUTTON_X) local_out_packet.buttons |= bp32_gamepad[idx].profile.x; 
            if (bt_gamepad->buttons & BUTTON_Y) local_out_packet.buttons |= bp32_gamepad[idx].profile.y; 

            if (bt_gamepad->buttons & BUTTON_THUMB_L)    local_out_packet.buttons |= bp32_gamepad[idx].profile.l3;
            if (bt_gamepad->buttons & BUTTON_THUMB_R)    local_out_packet.buttons |= bp32_gamepad[idx].profile.r3;
            if (bt_gamepad->buttons & BUTTON_SHOULDER_L) local_out_packet.buttons |= bp32_gamepad[idx].profile.lb;
            if (bt_gamepad->buttons & BUTTON_SHOULDER_R) local_out_packet.buttons |= bp32_gamepad[idx].profile.rb;

            if (bt_gamepad->misc_buttons & MISC_BUTTON_BACK)    local_out_packet.buttons |= bp32_gamepad[idx].profile.back;
            if (bt_gamepad->misc_buttons & MISC_BUTTON_START)   local_out_packet.buttons |= bp32_gamepad[idx].profile.start;
            if (bt_gamepad->misc_buttons & MISC_BUTTON_SYSTEM)  local_out_packet.buttons |= bp32_gamepad[idx].profile.sys;
            if (bt_gamepad->misc_buttons & MISC_BUTTON_CAPTURE) local_out_packet.buttons |= bp32_gamepad[idx].profile.misc;

            local_out_packet.lt = scale_bp_trigger(bt_gamepad->brake,    bp32_gamepad[idx].profile.deadzone.lt);
            local_out_packet.rt = scale_bp_trigger(bt_gamepad->throttle, bp32_gamepad[idx].profile.deadzone.rt);

            if (local_out_packet.lt < 1 && bt_gamepad->buttons & BUTTON_TRIGGER_L) local_out_packet.lt = UINT8_MAX;
            if (local_out_packet.rt < 1 && bt_gamepad->buttons & BUTTON_TRIGGER_R) local_out_packet.rt = UINT8_MAX;

            local_out_packet.ly = scale_bp_axis(bt_gamepad->axis_y,  bp32_gamepad[idx].profile.deadzone.ly, !bp32_gamepad[idx].profile.ly_invert);
            local_out_packet.lx = scale_bp_axis(bt_gamepad->axis_x,  bp32_gamepad[idx].profile.deadzone.ly, false);
            local_out_packet.ry = scale_bp_axis(bt_gamepad->axis_ry, bp32_gamepad[idx].profile.deadzone.ry, !bp32_gamepad[idx].profile.ry_invert);
            local_out_packet.rx = scale_bp_axis(bt_gamepad->axis_rx, bp32_gamepad[idx].profile.deadzone.ry, false);

            local_out_packet.packet_id = (uint8_t)PACKET_ID_GAMEPAD;
            local_out_packet.packet_size = (uint8_t)sizeof(i2cOutPacket);
            local_out_packet.input_mode_id = bp32_gamepad[idx].input_mode_id; // slaves 2-4 get their input mode from slave 1

            bp32_gamepad[idx].i2c_out_packet = local_out_packet;
            bp32_gamepad[idx].new_out_packet = true;

            break;
        default:
            break;
    }
}

const uni_property_t* my_platform_get_property(uni_property_idx_t idx) 
{
    ARG_UNUSED(idx);
    return NULL;
}

void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) 
{
    switch (event) 
    {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON: 
        {
            uni_hid_device_t* d = reinterpret_cast<uni_hid_device_t*>(data);

            if (d == NULL) 
            {
                loge("ERROR: OOB Event: Invalid NULL device\n");
                return;
            }

            break;
        }

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            logi("OOB Event: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("OOB Event: unsupported event: 0x%04x\n", event);
            break;
    }
}

void process_in_packet(int idx, i2cInPacket in_packet)
{
    static unsigned long last_rumble[MAX_GAMEPADS] = {};
    static const int rumble_length_ms = 150;

    if (in_packet.packet_size == sizeof(i2cInPacket) && idx == 0 && MAX_GAMEPADS > 1)
    {
        for (int i = 1; i < MAX_GAMEPADS; i++)
        {
            bp32_gamepad[i].input_mode_id = in_packet.input_mode_id;
        }
    }
    
    if (in_packet.packet_id == PACKET_ID_RUMBLE &&
        in_packet.packet_size == sizeof(i2cInPacket) &&
        bp32_gamepad[idx].device_ptr != nullptr)
    {
        unsigned long current_ms = esp_timer_get_time() / 1000;

        if (((in_packet.rumble_l + in_packet.rumble_r) > 0) &&
            current_ms - last_rumble[idx] >= (rumble_length_ms + 1))
        {
            #if (OGXW_DEBUG > 0)
            printf("IDX: %d | rumble right: %d | rumble left: %d\n", idx, in_packet.rumble_r, in_packet.rumble_l);
            #endif

            bp32_gamepad[idx].device_ptr->report_parser.play_dual_rumble(   bp32_gamepad[idx].device_ptr, 
                                                                            0, 
                                                                            rumble_length_ms, 
                                                                            in_packet.rumble_r, 
                                                                            in_packet.rumble_l);

            // vTaskDelay(pdMS_TO_TICKS(1));
            last_rumble[idx] = esp_timer_get_time() / 1000;
        }
    }
}

//
// Entry Point
//
struct uni_platform* get_my_platform(void) 
{
    static struct uni_platform plat = 
    {
        .name = "custom",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
        .on_oob_event = my_platform_on_oob_event,
    };

    return &plat;
}

void bp32_task(void* param)
{
    (void) param;

    init_user_settings();

    // Don't use BTstack buffered UART. It conflicts with the console.
// #ifdef CONFIG_ESP_CONSOLE_UART
// #ifndef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    // btstack_stdio_init();
// #endif  // CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
// #endif  // CONFIG_ESP_CONSOLE_UART

    btstack_init();

    uni_platform_set_custom(get_my_platform()); // Must be called before uni_init()

    uni_init(0 /* argc */, NULL /* argv */); // Init Bluepad32

    init_ble_server();

    btstack_run_loop_execute();
}