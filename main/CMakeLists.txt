idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash
                                     esp_driver_spi
                                     aht20
                                     nvs_flash
                                     wifi_manager
                                     mqtt_service
                       INCLUDE_DIRS ""
)
