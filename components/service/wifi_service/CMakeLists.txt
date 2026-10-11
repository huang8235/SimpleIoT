idf_component_register(SRCS "wifi_service.c"
                    PRIV_REQUIRES esp_wifi
                    REQUIRES esp_event
                    INCLUDE_DIRS "include")
