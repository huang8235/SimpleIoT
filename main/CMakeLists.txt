idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES iot_board
                                     iot_app
                                     esp_netif
                                     nvs_flash
                                     wifi_service
                                     mqtt_service
                                     ota_service
                       INCLUDE_DIRS ""
)
