#ifndef I2C_TASK_H_
#define I2C_TASK_H_

enum PacketID
{
    PACKET_ID_DISCONNECT = 0x10,
    PACKET_ID_GAMEPAD = 0x01,
    PACKET_ID_RUMBLE = 0x02,
};

enum InputMode
{
    INPUT_MODE_XINPUT           = 0x01,
    INPUT_MODE_SWITCH           = 0x02,
    INPUT_MODE_HID              = 0x03,
    INPUT_MODE_PSCLASSIC        = 0x04,
    INPUT_MODE_XBOXORIGINAL     = 0x05,
    INPUT_MODE_USBSERIAL        = 0x06,
    INPUT_MODE_UART_PASSTHROUGH = 0x07,
};

struct __attribute__((packed)) i2cOutPacket 
{
    uint8_t  packet_id      {0};
    uint8_t  input_mode_id  {0};
    uint8_t  packet_size    {0};
    uint16_t buttons        {0};
    uint8_t  lt {0};
    uint8_t  rt {0};
    int16_t  ly {0};
    int16_t  lx {0};
    int16_t  ry {0};
    int16_t  rx {0};
};

struct __attribute__((packed)) i2cInPacket 
{
    uint8_t packet_id     {0};
    uint8_t input_mode_id {0};
    uint8_t packet_size   {0};
    uint8_t rumble_l      {0};
    uint8_t rumble_r      {0};
};

#endif // I2C_TASK_H_