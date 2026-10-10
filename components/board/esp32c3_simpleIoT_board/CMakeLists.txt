idf_component_register(SRCS "esp32c3_simpleIoT_board.c"
                       PRIV_REQUIRES esp_driver_i2c
                                     esp_driver_spi
                       REQUIRES      aht20
                                     ssd1306
                       INCLUDE_DIRS "include")
