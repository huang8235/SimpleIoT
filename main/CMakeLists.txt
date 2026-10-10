idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES iot_board
                                     iot_app
                                     nvs_flash
                                     wifi_service
                                     mqtt_service
                       INCLUDE_DIRS ""
)
