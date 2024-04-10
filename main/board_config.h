#include "sdkconfig.h"

#define OGXW_RETAIL_1CH 1
#define OGXW_RPZERO_1CH 0
#define OGXW_RPZERO_2CH 0
#define OGXW_RPZERO_4CH 0
#define OGXW_RPZERO_4CH_PROTOTYPE 0

#define OGXW_DEBUG 0

#define FIRMWARE_NAME       "OGX-Wireless"
#define FIRMWARE_VERSION    "v1.0.0"

#define MAX_GAMEPADS CONFIG_BLUEPAD32_MAX_DEVICES

#if (OGXW_RETAIL_1CH > 0)
    #define RESET_BT_PIN 9

    #define I2C_MASTER_SCL_IO 22
    #define I2C_MASTER_SDA_IO 21

    #define LED_INDICATOR_PINS {15}
    // #define LED_INDICATOR_PINS {15, 32}

#elif (OGXW_RPZERO_1CH > 0) 
    #define RESET_BT_PIN 19

    #define I2C_MASTER_SCL_IO 22
    #define I2C_MASTER_SDA_IO 21

    #define LED_INDICATOR_PINS {32}

#elif (OGXW_RPZERO_2CH > 0) 
    #define RESET_BT_PIN 19

    #define I2C_MASTER_SCL_IO 22
    #define I2C_MASTER_SDA_IO 21

    // #define LED_INDICATOR_PINS {32, 33}
    #define LED_INDICATOR_PINS {25, 26}
    
#elif (OGXW_RPZERO_4CH > 0) 
    #define RESET_BT_PIN 19

    #define I2C_MASTER_SCL_IO 22
    #define I2C_MASTER_SDA_IO 21

    #define LED_INDICATOR_PINS {32, 33, 25, 26}

#elif (OGXW_RPZERO_4CH_PROTOTYPE > 0) 
    #define RESET_BT_PIN 19

    #define I2C_MASTER_SCL_IO 22
    #define I2C_MASTER_SDA_IO 21

    #define LED_INDICATOR_PINS {25, 26, 32, 33}

#endif