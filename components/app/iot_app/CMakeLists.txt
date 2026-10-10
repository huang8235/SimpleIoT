idf_component_register(SRCS "iot_app.c"
                            "oled_ui.c"
                       PRIV_REQUIRES esp32c3_simpleIoT_board
                                     mqtt_service
                       INCLUDE_DIRS "include")
