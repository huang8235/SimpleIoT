idf_component_register(SRCS "ota_service.c"
                    PRIV_REQUIRES app_update esp_https_ota mbedtls esp_wifi
                                  mqtt_service
                    INCLUDE_DIRS "include")
