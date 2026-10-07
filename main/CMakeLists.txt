idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash
                                     esp_driver_spi
                                     aht20
                                     wifi_manager
                                     nvs_flash
                       INCLUDE_DIRS ""
)
