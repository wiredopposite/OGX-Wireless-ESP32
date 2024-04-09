idf_component_register(SRCS "main.c" 
                            "bp32_task.cpp"
                            "i2c_task.cpp"
                            "gpio_task.cpp"
                            
                            "user_settings/user_settings.cpp"
                            "user_settings/nvs_helpers.cpp"

                            "ble_server/ble_server.cpp"

                            "utilities/scaling.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES bluepad32 btstack driver nvs_flash)
