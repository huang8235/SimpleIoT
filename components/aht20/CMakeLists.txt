idf_component_register(
    SRCS "aht20.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    PRIV_REQUIRES esp_driver_gpio
    REQUIRES esp_driver_i2c
)
